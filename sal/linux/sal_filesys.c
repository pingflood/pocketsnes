#include <string.h>
//#include <error.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "sal.h"

s32 sal_DirectoryGetCurrent(s8 *path, u32 size)
{
	getcwd(path,size);
	return SAL_OK;
}

s32 sal_DirectoryCreate(const char *path)
{
	s32 count=0;
	mkdir(path, 0x777);
	return SAL_OK;
}

s32 sal_DirectoryGetItemCount(const char *path, s32 *returnItemCount)
{
	u32 count=0;
	DIR *d;
	struct dirent *de;

	d = opendir(path);

	if (d)
	{
		while ((de = readdir(d)))
		{
			count++;
		}

	}
	else
		return SAL_ERROR;

	*returnItemCount=count;
	return SAL_OK;
}

s32 sal_DirectoryOpen(const char *path, struct SAL_DIR *d)
{
	d->dir=opendir(path);

	if(d->dir) return SAL_OK;
	else return SAL_ERROR;
}

s32 sal_DirectoryClose(struct SAL_DIR *d)
{
	if(d)
	{
		if(d->dir)
		{
			closedir(d->dir);
			d->dir=NULL;
			return SAL_OK;
		}
		else
		{
			return SAL_ERROR;
		}
	}
	else
	{
		return SAL_ERROR;
	}
}

s32 sal_DirectoryRead(struct SAL_DIR *d, struct SAL_DIRECTORY_ENTRY *dir, s8 *base_dir)
{
	struct dirent *de=NULL;
	struct stat s;
	char path[PATH_MAX];

	if(d)
	{
		if(dir)
		{
			de=readdir(d->dir);
			if(de)
			{
				sprintf(path, "%s/%s", base_dir, de->d_name);
				strcpy(dir->filename,de->d_name);
				strcpy(dir->displayName,de->d_name);
				if (stat(path, &s) != 0) return SAL_ERROR;
				if (s.st_mode & S_IFDIR)
				  dir->type=SAL_FILE_TYPE_DIRECTORY;
				else
				  dir->type=SAL_FILE_TYPE_FILE;
				return SAL_OK;
			}
			else
			{
				return SAL_ERROR;
			}
		}
		else
		{
			return SAL_ERROR;
		}
	}
	else
	{
		return SAL_ERROR;
	}
}

void sal_DirectoryGetParent(s8 *path)
{
	int i;
	unsigned int len = strlen(path);

	/* Strip trailing slashes */
	for (i = len - 1; i && (path[i] == SAL_DIR_SEP[0] ||
					path[i] == SAL_DIR_SEP_BAD[0]); i--)
		path[i] = '\0';

	for (; i && path[i] != SAL_DIR_SEP[0] && path[i] != SAL_DIR_SEP_BAD[0]; i--);
	if (i)
		path[i] = '\0';
	else
		path[i + 1] = '\0';
}
