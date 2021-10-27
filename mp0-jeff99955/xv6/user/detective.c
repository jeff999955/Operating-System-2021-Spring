#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int set;

char* fmtname(char *path)
{
	static char buf[DIRSIZ+1];
	char *p;

	// Find first character after last slash.
	for(p=path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;
	return p;

	// Return blank-padded name.
	if(strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
	return buf;
}

void dfs(char *path, char *query)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	if(st.type == T_FILE){
		if (!strcmp(query, fmtname(path))) {
			set = 1;
			printf("%d as Waston: %s\n", getpid(), path);
		}
	} else if (st.type == T_DIR) {
		if (!strcmp(query, fmtname(path))) {
			set = 1;
			printf("%d as Waston: %s\n", getpid(), path);
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if (!strcmp(".", de.name) || !strcmp("..", de.name)) continue;
			dfs(buf, query);
		}
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	int p[2];
	pipe(p);
	int pid;
	if ((pid = fork()) == 0) {
		close(p[0]);
		dfs(".", argv[1]);
		if (set) write(p[1], "y", 1);
		else write(p[1], "n", 1);
		exit(0);
	}
	wait((int *)0);
	char c;
	read(p[0], &c, 1);
	if (c == 'y') printf("%d as Holmes: This is the evidence\n",getpid());
	else if (c == 'n') printf("%d as Holmes: This is the alibi\n", getpid());
	else printf("error");
	exit(0);
}
