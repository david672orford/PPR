/*
** mouse:~ppr/src/pprd/pprd_question.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 8 January 2002.
*/

#include "before_system.h"
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprd.h"
#include "./pprd.auto_h"

/* How many processes may we use to ask questions? */
#define MAX_ACTIVE_QUESTIONS 10

/* This array of structures keeps track of the processes
   asking the questions. */
static struct {
    pid_t pid;
    int destnode_id;
    int destid;
    int id;
    int subid;
    int homenode_id;
    } active_question[MAX_ACTIVE_QUESTIONS];

/* How many entries in the above array user in use? */
static int active_questions = 0;

/* How many questions are still unanswered? */
static int outstanding_questions = 0;

/* These functions and values protect the integrity of the tables
   from disruption by the SIGCHLD handler calling question_child_hook().
   */
static int question_lock_level = 0;
static sigset_t question_block_set, question_save_set;
static void question_lock(void)
    {
    question_lock_level++;
    if(question_lock_level == 1)
	{
	sigemptyset(&question_block_set);
	sigaddset(&question_block_set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &question_block_set, &question_save_set);
	}
    }
static void question_unlock(void)
    {
    const char function[] = "question_unlock";
    question_lock_level--;
    if(question_lock_level == 0)
	sigprocmask(SIG_SETMASK, &question_block_set, (sigset_t*)NULL);
    if(question_lock_level < 0)
    	fatal(0, "%s(): assertion failed, question_lock_level=%d", function, question_lock_level);
    }

/*
** Initialize the question subsystem.
** This involves clearing all of the slots.
*/
void question_init(void)
    {
    FUNCTION4DEBUG("question_init")
    int x;
    DODEBUG_QUESTIONS(("%s()", function));
    for(x=0; x < MAX_ACTIVE_QUESTIONS; x++)
	{
	active_question[x].pid = 0;
	}
    }

/*
** Launch a questioner process.
*/
static int question_launch(struct QEntry *job)
    {
    const char function[] = "question_launch";
    int x;

    DODEBUG_QUESTIONS(("%s(job={id=%d})", function, job->id));

    if(question_lock_level == 0)
    	fatal(0, "%s(): tables not locked", function);

    /* Find the first empty slot.  We know there is one. */
    for(x=0; x < MAX_ACTIVE_QUESTIONS; x++)
	{
	if(active_question[x].pid == 0)
	    break;
	}
    if(x == MAX_ACTIVE_QUESTIONS)
    	fatal(0, "%s(): assertion failed", function);
    DODEBUG_QUESTIONS(("%s(): slot %d is free", function, x));

    if((active_question[x].pid = fork()) == -1)
    	{
	error("%s(): fork() failed, errno=%d (%s)", function, errno, strerror(errno));
	return -1;
    	}

    /* If child, */
    if(active_question[x].pid == 0)
	{
	char jobname[256];
        char filename[MAX_PPR_PATH];
        FILE *qfile;
        char *line = NULL;
        int line_available = 80;
        char *response_responder = NULL, *response_address = NULL, *response_options = NULL;
        char *question = NULL, *magic_cookie = NULL, *title = "";

	child_unblock_all();
	child_stdin_stdout_stderr("/dev/null", PPRD_LOGFILE);

	snprintf(jobname, sizeof(jobname), "%s:%s-%d.%d(%s)", ppr_get_nodename(), destid_to_name(job->destnode_id, job->destid), job->id, job->subid, nodeid_to_name(job->homenode_id));
        ppr_fnamef(filename, "%s/%s", QUEUEDIR, jobname);
        if(!(qfile = fopen(filename, "r")))
            {
            error("%s(): can't open \"%s\", errno=%d (%s)", function, filename, errno, strerror(errno));
            exit(10);
            }

        while((line = gu_getline(line, &line_available, qfile)))
            {
	    switch(line[0])
		{
		case 'Q':
	            if(gu_sscanf(line, "Question: %Z", &question) == 1)
			continue;
		    break;
		case 'R':
		    if(gu_sscanf(line, "Response: %S %S %Z", &response_responder, &response_address, &response_options) >= 2)
			continue;
		    break;
		case 'M':
		    if(gu_sscanf(line, "MagicCookie: %Z", &magic_cookie) == 1)
		    	continue;
		    break;
		case 'T':
		    if(gu_sscanf(line, "Title: %Z", &title) == 1)
		    	continue;
		    break;
		}
            }
        if(line)
	    gu_free(line);

        fclose(qfile);

	if(!question || !response_responder || !response_address)
	    {
	    debug("child: %s(): required parameter missing", function);
	    exit(11);
	    }

	execl("lib/pprd-question", "pprd-question", response_responder, response_address, response_options ? response_options : "", question, jobname, magic_cookie, title, NULL);
	debug("child: %s(): execl() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	exit(12);
	}

    DODEBUG_QUESTIONS(("%s(): pid is %ld", function, (long)active_question[x].pid));

    /* Set a flag so we don't launch more than one questioner. */
    job->flags |= JOB_FLAG_QUESTION_ASKING_NOW;

    /* Set information so we can find the job. */
    active_question[x].destnode_id = job->destnode_id;
    active_question[x].destid = job->destid;
    active_question[x].id = job->id;
    active_question[x].subid = job->subid;
    active_question[x].homenode_id = job->homenode_id;

    active_questions++;

    DODEBUG_QUESTIONS(("%s()", function));
    return 0;
    } /* end of question_launch() */

/*
** Find a question waiting to be asked.  We only go so far as we need to
** to find all of the outstanding questions.  Since outstanding_questions
** isn't adjusted when a job with an outstanding question is deleted,
** we compute a new one as we go.
*/
static void question_look_for_work(void)
    {
    FUNCTION4DEBUG("question_look_for_work")
    DODEBUG_QUESTIONS(("%s(): outstanding_questions=%d", function, outstanding_questions));
    if(outstanding_questions > 0)
	{
	int x, count;
	time_t time_now;

	time(&time_now);

	DODEBUG_QUESTIONS(("%s(): time_now=%ld", function, (long)time_now));

	question_lock();

	for(x=count=0; x < queue_entries && count < outstanding_questions && active_questions < MAX_ACTIVE_QUESTIONS; x++)
	    {
	    DODEBUG_QUESTIONS(("%s(): id=%d, UNANSWERED=%s, ASKING_NOW=%s, resend_message_at=%ld (now %+ld)",
	    	function,
		queue[x].id,
                queue[x].flags & JOB_FLAG_QUESTION_UNANSWERED ? "YES" : "NO",
                queue[x].flags & JOB_FLAG_QUESTION_ASKING_NOW ? "YES" : "NO",
                (long)queue[x].resend_message_at,
                (long)(queue[x].resend_message_at - time_now)
	    	));
	    if(queue[x].flags & JOB_FLAG_QUESTION_UNANSWERED)
		{
		count++;
	    	if(!(queue[x].flags & JOB_FLAG_QUESTION_ASKING_NOW)
	    		&& queue[x].resend_message_at <= time_now)
		    {
		    question_launch(&queue[x]);
		    }
	    	}
	    }

	if(x==queue_entries)
	    outstanding_questions = count;

        question_unlock();
	}

    DODEBUG_QUESTIONS(("%s(): done", function));
    } /* end of question_look_for_work() */

/*
** This is called whenever a job with an outstanding question is inserted
** into the queue.  We try to launch a questioner processor right away,
** but if we can't we increment the count of the number of questions that
** would like to go.
*/
void question_job(struct QEntry *job)
    {
    FUNCTION4DEBUG("question_job")
    DODEBUG_QUESTIONS(("%s(job={id=%d})", function, job->id));
    question_lock();
    outstanding_questions++;
    if(active_questions < MAX_ACTIVE_QUESTIONS)
	question_launch(job);
    question_unlock();
    DODEBUG_QUESTIONS(("%s(): done", function));
    } /* end of question_job() */

/*
** This is called from reapchild().  We get to claim our children.
*/
gu_boolean question_child_hook(pid_t pid, int wstat)
    {
    const char function[] = "question_child_hook";
    int x;
    gu_boolean retval = FALSE;

    DODEBUG_QUESTIONS(("%s(pid=%ld, wstat=0x%08X)", function, (long)pid, wstat));

    question_lock();

    /* Look for an active question with this PID. */
    for(x=0; x < MAX_ACTIVE_QUESTIONS; x++)
	{
	DODEBUG_QUESTIONS(("%s(): match at slot %d, job %s:%s-%d.%d(%s)",
		function,
		x,
		nodeid_to_name(active_question[x].destnode_id),
		destid_to_name(active_question[x].destnode_id, active_question[x].destid),
		active_question[x].id,
		active_question[x].subid,
		nodeid_to_name(active_question[x].homenode_id) ));
	if(active_question[x].pid == pid)
	    {
	    /* Find the queue entry it relates to. */
	    int y;
	    for(y=0; y < queue_entries; y++)
		{
		if(	queue[y].id == active_question[x].id
			&& queue[y].destid == active_question[x].destid
			&& queue[y].subid == active_question[x].subid
			&& queue[y].destnode_id == active_question[x].destnode_id
			&& queue[y].homenode_id == active_question[x].homenode_id)
		    {
		    time_t time_now;

		    /* OK, now we have both */
                    DODEBUG_QUESTIONS(("%s(): match with job %d", function, queue[y].id));

		    if(!(queue[y].flags & JOB_FLAG_QUESTION_ASKING_NOW))
		    	fatal(0, "%s(): assertion failed: ASKING_NOW not set", function);

		    time(&time_now);

		    if(WIFEXITED(wstat) && WEXITSTATUS(wstat) == 0)
			{
			DODEBUG_QUESTIONS(("%s(): success, repeat in 300 seconds", function));
			queue[y].resend_message_at = time_now + 300;
			}
		    else
		    	{
			DODEBUG_QUESTIONS(("%s(): failure, retry in 60 seconds", function));
		    	queue[y].resend_message_at = time_now + 60;
		    	}

		    queue[y].flags &= ~JOB_FLAG_QUESTION_ASKING_NOW;

		    break;
		    }
		}
	    #ifdef DEBUG_QUESTIONS
	    if(y == queue_entries)
	    	debug("%s(): job no longer in queue", function);
	    #endif

	    active_question[x].pid = 0;
	    active_questions--;
	    question_look_for_work();
	    retval = TRUE;
	    break;
	    }
	}

    question_unlock();

    DODEBUG_QUESTIONS(("%s(): returning %s", function, retval ? "TRUE" : "FALSE"));
    return retval;
    } /* end of question_child_hook() */

/*
** This is called every so often so we can retry questions.
*/
void question_tick(void)
    {
    FUNCTION4DEBUG("question_tick")
    DODEBUG_QUESTIONS(("%s()", function));
    question_look_for_work();
    DODEBUG_QUESTIONS(("%s(): done", function));
    }

/* end of file */
