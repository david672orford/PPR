/*
** mouse:~ppr/src/ppr-papd/ppr-papd_conf.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 10 January 2003.
*/

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <dirent.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"

/*
** The code in this module opens each alias, group, and printer config file 
** and gets a list of the AppleTalk names to advertise.
*/

/*
** The DIRS structure is used to describe a list of directories which contain
** the configuration files for PPR queues of various types.
*/

struct DIRS {
    const char *name;
    enum QUEUEINFO_TYPE type;
    } ;

static struct DIRS dirs[] = {
    {ALIASCONF, QUEUEINFO_ALIAS},
    {GRCONF, QUEUEINFO_GROUP},
    {PRCONF, QUEUEINFO_PRINTER}
    };

/*
** This is called for each config file that conf_load() finds.
*/
static struct ADV *do_config_file(struct ADV *adv, enum QUEUEINFO_TYPE qtype, const char qname[], FILE *f)
    {
    char *line = NULL;
    int line_available = 80;
    char *p;
    char *papname = NULL;

    /*
    ** Search the configuration file for a line the specifies an AppleTalk name
    ** on which to provide PAP services.
    */
    while((line = gu_getline(line, &line_available, f)))
	{
	if((p = lmatchp(line, "ppr-papd papname:")))
	    {
	    if(papname)
	    	gu_free(papname);
	    papname = gu_strdup(p);
	    }
	}

    /*
    ** If we found a PAP name, we take action.  If this queue had used to 
    ** have a PAP name, that is ok because conf_load() will notice that 
    ** we didn't switch the adv_type back to ADV_ACTIVE and will
    ** get rid of it.
    */
    if(papname)
	{
	int i = 0;
	
	/*
	** If the array is already allocated, look for an existing entry, 
	** or failing that, a deleted entry to reuse.
	*/
	if(adv)
	    {
	    int freespot = -1;
	    for(i = 0; adv[i].adv_type != ADV_LAST; i++)
		{
		if(adv[i].adv_type == ADV_DELETED)
		    freespot = i;
		else if(adv[i].adv_type == qtype && strcmp(adv[i].PPRname, qname) == 0)
		    break;
	    	}
	    if(adv[i].adv_type == ADV_LAST)
	        {
	        if(freespot != -1)
		    i = freespot;
		}
	    }

	/* If we didn't find an existing entry, expand the array. */
	if(!adv || adv[i].adv_type == ADV_LAST)
	    {
	    adv = (struct ADV *)gu_realloc(adv, i + 2, sizeof(struct ADV));
	    adv[i+1].adv_type = ADV_LAST;
	    adv[i].PPRname = NULL;
	    adv[i].PAPname = NULL;
	    }

	/* If this is a new entry, its name and type won't have been stored yet. */
	if(!adv[i].PPRname)
	    {
	    adv[i].queue_type = qtype;
	    adv[i].PPRname = gu_strdup(qname);
	    }

	/*
	** If there is an existing advertized name and it differs from what we
	** just read from the config file, then remove it from the network
	** and clear it from the list.
	*/
	if(adv[i].PAPname && strcmp(adv[i].PAPname, papname) != 0)
	    {
	    remove_name(adv[i].PAPname, adv[i].fd);
	    gu_free((char*)adv[i].PAPname);
	    adv[i].PAPname = NULL;
	    }
	    
	/* If we haven't an advertised PAP name, add it to the AppleTalk network. */
	if(!adv[i].PAPname)
	    {
	    adv[i].fd = add_name(papname);
	    adv[i].PAPname = papname;
	    }
	else
	    {
	    gu_free(papname);
	    }

	adv[i].adv_type = ADV_ACTIVE;
	}

    return adv;
    }

/*
** This function is called from main().
*/
struct ADV *conf_load(struct ADV *adv)
    {
    const char function[] = "read_conf";
    int i;
    struct DIRS *dir;
    DIR *dirobj;
    struct dirent *direntp;
    char fname[MAX_PPR_PATH];
    int len;
    FILE *f;
    
    /*
    ** Mark all of the active advertised names with a special temporary 
    ** status.  Any which are still present in the config file will
    ** get changed back to ADV_ACTIVE.  Those which aren't changed back
    ** will be deleted from the network later.
    */
    for(i = 0; adv[i].adv_type != ADV_LAST; i++)
    	{
	if(adv[i].adv_type == ADV_ACTIVE)
	    adv[i].adv_type = ADV_RELOADING;
    	}

    /*
    ** Step through the alias, group, and printer configuration directories
    ** processing each file.
    */
    for(dir=dirs; dir->name; dir++)
	{
	DODEBUG_STARTUP(("%s", dir->name));
	if(!(dirobj = opendir(dir->name)))
	    fatal(0, _("%s(): opendir(\"%s\") failed, errno=%d (%s)"), function, dir, errno, gu_strerror(errno));
	while((direntp = readdir(dirobj)))
	    {
	    /* Skip . and .. and hidden files. */
	    if(direntp->d_name[0] == '.')
		continue;

	    /* Skip Emacs style backup files. */
	    len = strlen(direntp->d_name);
	    if(len > 0 && direntp->d_name[len-1] == '~')
		continue;

	    DODEBUG_STARTUP(("  %s", direntp->d_name));
	    ppr_fnamef(fname, "%s/%s", dir->name, direntp->d_name);
	    if(!(f = fopen(fname, "r")))
	    	fatal(0, "%s(): fopen(\"%s\", \"r\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));

	    adv = do_config_file(adv, dir->type, direntp->d_name, f);

	    fclose(f);
	    }
	closedir(dirobj);
	}

    /*
    ** Unadvertise any entries which were set to ADV_RELOADING above and 
    ** didn't get changed back to ADV_ACTIVE.
    */
    for(i = 0; adv[i].adv_type != ADV_LAST; i++)
    	{
	if(adv[i].adv_type == ADV_RELOADING)
	    {
	    remove_name(adv[i].PAPname, adv[i].fd);
	    gu_free((char*)adv[i].PPRname);
	    adv[i].PPRname = NULL;
	    gu_free((char*)adv[i].PAPname);
	    adv[i].PAPname = NULL;
	    adv[i].adv_type = ADV_DELETED;
	    }
    	}

    return adv;
    }

/*
**
*/
void conf_load_queue_config(struct ADV *adv, struct QUEUE_CONFIG *qc)
    {
    qc->PPDfile = NULL;
    qc->fontlist = (const char**)NULL;
    qc->fontcount = 0;
    qc->LanguageLevel = 1;
    qc->PSVersion = "(50.0) 0";
    qc->Resolution = "300dpi";
    qc->BinaryOK = FALSE;
    qc->FreeVM = 600000;
    qc->InstalledMemory = NULL;
    qc->VMOptionFreeVM = 600000;
    qc->Product = NULL;
    qc->ColorDevice = FALSE;
    qc->RamSize = 0;
    qc->FaxSupport = NULL;
    qc->TTRasterizer = NULL;
    qc->options = NULL;
    qc->query_font_cache = TRUE;
    }

/* end of file */
