#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sched.h>
#include<sys/syscall.h>
#include<unistd.h>

#define MAX_PROCESS_COUNT 255

struct sched_attr {
	uint32_t size; // size of structure
	uint32_t sched_policy; // policy
	uint64_t sched_flags; // flag?
	int32_t sched_nice; // nice value only for other
	uint32_t sched_priority; // RT priority
	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
};

typedef struct {
	char container_name[MAX_PROCESS_COUNT];
	char name[MAX_PROCESS_COUNT];
	int policy;
	int priority;
	int enable_lwp;
} process_attr;

int setProcesses(FILE *pFile, int* interval, process_attr** process_list);
void setPriority(int policy, int pid, int priority);
static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags);
void printProcesses(process_attr* process_list, int process_count);
const char* docker_uri;

int main(int argc, char* argv[]) {
	docker_uri = getenv("DOCKER_URI");
	printf("docker uri : %s\n", docker_uri);
	int interval, i, j;
	int process_count;
	int flag = 1;
	process_attr* process_list = NULL;
	FILE *pFile;

	if (pFile = fopen("./config.txt", "r")) {
		process_count = setProcesses(pFile, &interval, &process_list);
		printProcesses(process_list, process_count);
	}

	while(1){
		usleep(interval * 1000);
		for(i = 0; i < process_count; i++){
			char cmd_dockertop[1024];// = (char*)malloc(100*sizeof(char)); // must be free
			char output_dockertop[1024];
			FILE *fp; // must be closed
			sprintf(cmd_dockertop, "docker -H %s top %s | grep %s | awk '{print $2}'", docker_uri,
					process_list[i].container_name, process_list[i].name);
			fp = popen(cmd_dockertop, "r");

			// get process id in container by one.
			while(1){
				if(fgets(output_dockertop, sizeof(output_dockertop)-1, fp) == NULL){
					break;
				}
				FILE *fp2;
				char cmd_ps[1024];
				char output_ps[1024];
				output_dockertop[strcspn(output_dockertop, "\n")] = 0; // remove included \n in output of cmd.
				sprintf(cmd_ps, "ps -cLe | grep %s | awk '{ print $4; print $3; print $2; print $1;}'", output_dockertop);
				// printf("%s\n", cmd_ps);
				fp2 = popen(cmd_ps, "r");
				flag = 1;

				while(flag){
					char priority_output[1024];
					char policy_output[1024];
					char lwp_pid_output[1024];
					char pid_output[1024];
					for(j = 0; j < 4; j++){
						if(fgets(output_ps, sizeof(output_ps)-1, fp2) == NULL){
							flag = 0;
							break;
						}
						output_ps[strcspn(output_ps, "\n")] = 0; // remove included \n in output of cmd.

						switch(j){
							case 0: {
								strcpy(priority_output, output_ps);
								break;
							}
							case 1: {
								strcpy(policy_output, output_ps);
								break;
							}
							case 2: {
								strcpy(lwp_pid_output, output_ps);
								break;
							}
							case 3: {
								strcpy(pid_output, output_ps);
								break;
							}
						}
					}

					if(flag){
						//printf("%d %d %d %d\n", process_list[i].policy, atoi(policy_output), process_list[i].priority , atoi(priority_output));
						if(process_list[i].policy == SCHED_OTHER && 19 - process_list[i].priority == atoi(priority_output)){
							//printf("pid %d is already set.\n", atoi(pid_output));
						}else if((process_list[i].policy == SCHED_FIFO ||
								process_list[i].policy == SCHED_RR) &&
								process_list[i].priority + 40 == atoi(priority_output)){
							//printf("pid %d is already set.\n", atoi(pid_output));
						}
						else{
							if(!(process_list[i].enable_lwp == 1 && atoi(lwp_pid_output) == atoi(pid_output))){
								setPriority(process_list[i].policy, atoi(lwp_pid_output), process_list[i].priority);
							}
							/*
							else{
								printf("%s %s is same!\n", lwp_pid_output, pid_output);
							}*/
						}
					}
				}

				pclose(fp2);
			}
			pclose(fp);
		}
	}
	return 0;
}

int setProcesses(FILE *pFile, int* interval, process_attr** process_list) {
	char buffer[MAX_PROCESS_COUNT];
	int value = fscanf(pFile, "%s", buffer);
	int flag = 1;
	int process_count = 0;
	int i;
	char* container_name[MAX_PROCESS_COUNT];
	char* name[MAX_PROCESS_COUNT];
	int policy[MAX_PROCESS_COUNT];
	int priority[MAX_PROCESS_COUNT];
	int enable_lwp[MAX_PROCESS_COUNT];

	*interval = atoi(buffer);
	printf("Scheduling interval : %d ms\n", *interval);

	while (flag) {
		for (i = 0; i < 5; i++) {
			value = fscanf(pFile, "%s", buffer);
			if (value == EOF) {
				flag = 0;
				break;
			}
			switch (i) {
				case 0:{
					container_name[process_count] = (char*)malloc(sizeof(char)*MAX_PROCESS_COUNT);
					strcpy(container_name[process_count], buffer);
					break;
				}
				case 1: {
					if (!strcmp(buffer, "SCHED_OTHER")) {
						policy[process_count] = 0;
					}
					else if (!strcmp(buffer, "SCHED_FIFO")) {
						policy[process_count] = 1;
					}
					else if (!strcmp(buffer, "SCHED_RR")) {
						policy[process_count] = 2;
					}
					else {
						printf("Invalid RT scheduling option : %s\n", buffer);
						exit(1);
					}
					break;
				}
				case 2: {
					priority[process_count] = atoi(buffer);
					break;
				}
				case 3: {
					name[process_count] = (char*)malloc(sizeof(char)*MAX_PROCESS_COUNT);
					strcpy(name[process_count], buffer);
					break;
				}
				case 4: {
					enable_lwp[process_count] = atoi(buffer);
					break;
				}
			}
		}
		if (value != EOF) {
			process_count++;
		}
	}
	*process_list = (process_attr*)malloc(sizeof(process_attr)*process_count);

	for (i = 0; i < process_count; i++) {
		//memset((*process_list)[i].name, 0, MAX_PROCESS_COUNT);
		strcpy((*process_list)[i].container_name, container_name[i]);
		strcpy((*process_list)[i].name, name[i]);
		(*process_list)[i].priority = priority[i];
		(*process_list)[i].policy = policy[i];
		(*process_list)[i].enable_lwp = enable_lwp[i];
	}

	return process_count;
}

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags){
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

void setPriority(int policy, int pid, int priority){
	int result;
	struct sched_attr attr;
	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(struct sched_attr);
	attr.sched_policy = policy;
	if(policy == SCHED_OTHER){
		attr.sched_nice = priority;
	}else{
		attr.sched_priority = priority;
	}
	result = sched_setattr(pid, &attr, 0);
	if(result == -1){
		perror("Error calling sched_setattr");
	}else{
		printf("Successfully set priority of pid : %d\n", pid);
	}
}

void printProcesses(process_attr* process_list, int process_count) {
	int i;
	for (i = 0; i < process_count; i++) {
		printf("Container : [%s], Process : [%s], Priority : [%d], Policy : [%d], Enable Lwp : %d \n",
				process_list[i].container_name, process_list[i].name, process_list[i].priority, process_list[i].policy,
				process_list[i].enable_lwp);
	}
}
