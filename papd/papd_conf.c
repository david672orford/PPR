/*
** mouse:~ppr/src/papd/papd_conf.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 11 April 2006.
*/

#include "config.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "queueinfo.h"
#include "papd.h"

/*============================================================================
** The code in this module opens each alias, group, and printer config file 
** and gets a list of the AppleTalk names to advertise.
============================================================================*/

/*
** The DIRS structure is used to describe a list of directories which contain
** the configuration files for PPR queues of various types.
*/

struct DIRS {
	const char *name;
	enum QUEUEINFO_TYPE type;
	} dirs[] = {
	{ALIASCONF, QUEUEINFO_ALIAS},
	{GRCONF, QUEUEINFO_GROUP},
	{PRCONF, QUEUEINFO_PRINTER},
	{NULL, QUEUEINFO_SEARCH}
	};

/*
** This function retrieves the default zone name from ppr.conf.
*/
static const char *default_zone(void)
	{
	static const char *dz = NULL;
	if(!dz)
		dz = gu_ini_query(PPR_CONF, "papd", "defaultzone", 0, "*");
	return dz;
	}

/*
** This is called for each alias, group, or printer configuration file that
** conf_load() finds.
*/
static struct ADV *do_config_file(struct ADV *adv, enum QUEUEINFO_TYPE qtype, const char qname[], FILE *f)
	{
	const char function[] = "do_config_file";
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
		if((p = lmatchp(line, "papname:")) && gu_sscanf(p, "%A", &p) == 1)
			{
			if(papname)
				gu_free(papname);
			papname = p;
			}
		}

	debug("| %-16s | %s", qname, papname ? papname : "");
	
	/*
	** Parse the AppleTalk name and insert defaults for missing parts.
	*/
	if(papname)
		{
		char *p = papname;
		const char *name = p;
		const char *type = NULL;
		const char *zone = NULL;
		int len;
		int c;

		len = strcspn(p, ":@");
		if((c = p[len]))
			{
			p[len] = '\0';
			p += len + 1;
			if(c == ':')
				{
				len = strcspn(p, "@");
				c = p[len];
				p[len] = '\0';
				type = p;
				p += len + 1;
				}
			if(c == '@')
				{
				zone = p;
				}
			}		

		if(!type)
			type = "LaserWriter";
		if(!zone)
			zone = default_zone();

		gu_asprintf(&p, "%s:%s@%s", name, type, zone);

		if(strlen(name) > 32 || strlen(name) < 1 || strlen(type) > 32 || strlen(type) < 1 || strlen(zone) > 32 || strlen(zone) < 1)
			{
			debug("%s(): papname \"%s\" has at least one component of invalid length", function, p);
			gu_free(p);
			p = NULL;
			}

		gu_free(papname);
		papname = p;
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
		
		DODEBUG_STARTUP(("%s(): papname[]=\"%s\"", function, papname));

		/*
		** Look for an existing entry for this queue, 
		** or failing that, a deleted entry to reuse.
		*/
		{
		int freespot = -1;
		for(i = 0; adv[i].adv_type != ADV_LAST; i++)
			{
			if(adv[i].adv_type == ADV_DELETED)
				{
				if(freespot == -1)		/* if first free spot, */
					freespot = i;
				}
			else if(adv[i].queue_type == qtype && strcmp(adv[i].PPRname, qname) == 0)
				{
				break;
				}
			}
		/* If we didn't find it, but did find a free spot, move back to the first one. */
		if(adv[i].adv_type == ADV_LAST)
			{
			if(freespot != -1)
				i = freespot;
			}
		}

		/* If we didn't find an existing entry or a blank space, expand the array. */
		if(adv[i].adv_type == ADV_LAST)
			{
			adv = (struct ADV *)gu_realloc(adv, i + 2, sizeof(struct ADV));
			adv[i+1].adv_type = ADV_LAST;
			adv[i+1].PPRname = NULL;
			adv[i+1].PAPname = NULL;
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
			debug("|                  | unbinding \"%s\" (old name)...", adv[i].PAPname);
			at_remove_name(adv[i].PAPname, adv[i].fd);
			gu_free((char*)adv[i].PAPname);
			adv[i].PAPname = NULL;
			}
			
		/* If we haven't an advertised PAP name, add it to the AppleTalk network. */
		if(!adv[i].PAPname)
			{
			debug("|                  | binding \"%s\"...", papname);
			adv[i].fd = at_add_name(papname);
			adv[i].PAPname = papname;
			}
		else		/* must be the same as the old one */
			{
			gu_free(papname);
			}

		adv[i].adv_type = ADV_ACTIVE;
		}

	return adv;
	} /* end of do_config_file() */

/*
** This function is called from main().
*/
struct ADV *conf_load(struct ADV *adv)
	{
	const char function[] = "conf_load";
	int i;
	struct DIRS *dir;
	DIR *dirobj;
	struct dirent *direntp;
	char fname[MAX_PPR_PATH];
	int len;
	FILE *f;
	
	DODEBUG_STARTUP(("%s(%p)", function, adv));
	debug("| Queue            | PAP Name");
	debug("+==================+========================================");

	if(!adv)
		{
		DODEBUG_STARTUP(("%s(): allocating initial adv[]", function));
		adv = gu_alloc(1, sizeof(struct ADV));
		adv[0].adv_type = ADV_LAST;
		adv[0].PPRname = NULL;
		adv[0].PAPname = NULL;

		/*
		** This Linux code sets up monitoring of the configuration directories.
		** Whenever one of the monitored directories changes, this process
		** will receive SIGIO.  A handler in papd.c will call this function
		** to reload the configuration.
		** 
		** This code will only be complied if we are using a suffiently new 
		** version of GNU libc.  The fcntl() call will fail on kernels
		** earlier than 2.4.19.
		*/
		#ifdef F_NOTIFY
		DODEBUG_STARTUP(("%s(): installing directory change handler", function));
		for(dir=dirs; dir->name; dir++)
			{
			int fd;
			if((fd = open(dir->name, O_RDONLY)) < 0)
				gu_Throw("%s(): open(\"%s\", O_RDONLY) failed, errno=%d (%s)", function, dir->name, errno, gu_strerror(errno));
			if(fcntl(fd, F_NOTIFY, DN_MULTISHOT | DN_MODIFY | DN_CREATE | DN_DELETE) == -1)
				{
				DODEBUG_STARTUP(("%s(): dnotify doesn't work", function));
				close(fd);
				break;
				}
			}
		#endif
		}
		
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
		DODEBUG_STARTUP(("%s(): searching directory %s", function, dir->name));

		if(!(dirobj = opendir(dir->name)))
			gu_Throw(_("%s(): opendir(\"%s\") failed, errno=%d (%s)"), function, dir->name, errno, gu_strerror(errno));

		while((direntp = readdir(dirobj)))
			{
			/* Skip . and .. and hidden files. */
			if(direntp->d_name[0] == '.')
				continue;

			/* Skip Emacs style backup files. */
			len = strlen(direntp->d_name);
			if(len > 0 && direntp->d_name[len-1] == '~')
				continue;

			DODEBUG_STARTUP(("%s():	   %s", function, direntp->d_name));

			ppr_fnamef(fname, "%s/%s", dir->name, direntp->d_name);
			if(!(f = fopen(fname, "r")))
				gu_Throw("%s(): fopen(\"%s\", \"r\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));

			adv = do_config_file(adv, dir->type, direntp->d_name, f);

			fclose(f);
			}

		closedir(dirobj);
		}

	debug("+==================+========================================");

	/*
	** Unadvertise any entries which were set to ADV_RELOADING above and 
	** didn't get changed back to ADV_ACTIVE.
	*/
	for(i = 0; adv[i].adv_type != ADV_LAST; i++)
		{
		if(adv[i].adv_type == ADV_RELOADING)
			{
			debug("%s: unbinding \"%s\" (no longer advertised)", adv[i].PPRname, adv[i].PAPname);
			at_remove_name(adv[i].PAPname, adv[i].fd);
			gu_free((char*)adv[i].PPRname);
			adv[i].PPRname = NULL;
			gu_free((char*)adv[i].PAPname);
			adv[i].PAPname = NULL;
			adv[i].adv_type = ADV_DELETED;
			}
		}

	DODEBUG_STARTUP(("%s(): done", function));

	return adv;
	} /* end of conf_load() */

/* end of file */
