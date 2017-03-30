/*
 * jzqload.c
 *
 *  Created on: 2013-6-8
 *      Author: yd
 */
#include "comm.h"
int SearchProcFile(char *filename, char *name, char *ret, char flag_serarch)
{
	FILE *fp;
	char stmp[100],sline[100];
	char *pstr;
	int strsize=0;
	fp=fopen(filename, "r");
	if(fp == NULL)
		return 0;
	while(!feof(fp))
	{
		if(fgets(stmp, 200, fp) != NULL)
		{
			strsize=strlen(stmp);
			if(stmp[strsize-1] == 0x0A)
				stmp[strsize-1]='\0';
			strcpy(sline,stmp);
			switch (flag_serarch)
			{
			case 0:				//match the whole word  (name)
				pstr = strtok(stmp," ");
				while(pstr)
				{
					if(strcmp(pstr, name) == 0)
					{
						strcpy(ret, sline);
						fclose(fp);
						return 1;
					}
					pstr = strtok(NULL," ");
				}
				break;
			case 1:          //contain the word (name)
				if(strstr(stmp, name) != NULL)
				{
					strcpy(ret, sline);
					fclose(fp);
					return 1;
				}
				break;
			default:
				break;
			}
		}
	}
	fclose(fp);
	return 0;
}

int getMemInfo()
{
	FILE *fp;
	long int totalmem=0;
	long int freemem=0;
	char itemname[30];
	long int data;
	char dw[5];
	int memused=0;
	fp = fopen("/proc/meminfo", "r");
	if(fp == NULL) return 0;
	while(!feof(fp))
	{
		fscanf(fp,"%s %ld %s", itemname, &data, dw);
		if(strcmp(itemname, "MemTotal:") == 0)
			totalmem = data;
		if(strcmp(itemname, "MemFree:") == 0)
			freemem = data;
	}
	fclose(fp);
	if(totalmem > freemem)
	memused = (float)(totalmem-freemem)/(float)totalmem *100;
	return memused;
}

int getNandInfo()
{
	char strret[100];
	char str1[30];
	long int in1, in2, in3;
	int usednand;
	system("df > /dev/shm/diskload.info");
	if(SearchProcFile((char*)"/dev/shm/diskload.info", (char*)"/nand", strret, 0) == 0)
	{
		printf("\nNo nand!!!\n");
		return 0;
	}
	sscanf(strret, "%s %ld %ld %ld %d",str1,&in1,&in2,&in3,&usednand);
	return usednand;
}

int getNorInfo()
{
	char strret[100];
	char str1[30];
	long int in1, in2, in3;
	int usednor;
	system("df > /dev/shm/diskload.info");
	if(SearchProcFile((char*)"/dev/shm/diskload.info", (char*)"/nor", strret, 0) == 0)
	{
		fprintf(stderr, "\nNo nor!!!\n");
		return 0;
	}
	sscanf(strret, "%s %ld %ld %ld %d",str1,&in1,&in2,&in3,&usednor);
	return usednor;
}

int getCpuInfo()
{
	FILE *fp;
	char itemname[30];
	long int data[9],total_jffies=0;
	int cpuused=0, i=0;

	fp = fopen("/proc/stat", "r");
	if(fp == NULL) return 0;
	while(!feof(fp))
	{
		fscanf(fp,"%s %ld %ld %ld %ld %ld %ld %ld %ld %ld",
				itemname, &data[0],&data[1],&data[2],&data[3],
				&data[4], &data[5],&data[6],&data[7],&data[8]);
		if(strcmp(itemname, (char*)"cpu") == 0)
		{
			for(i=0; i<9;i++)
				total_jffies += data[i];
			cpuused = (1-(float)data[3]/(float)total_jffies)*100;
			if(cpuused==0)
				cpuused=1;
		}
	}
	fclose(fp);
	return cpuused;
}
