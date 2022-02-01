#include <sstream>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#include <zlib.h>

using namespace std;

#define TASK_DL_TABLE 1
#define TASK_DL_DETAILS 2
#define TASK_DL_IMAGES 3
#define TASK_PAID 4
#define SAVE_TABLE 5
#define LOAD_TABLE 6
#define IMPORT_TAXMAP_LIST 7
#define EXPORT_TABLE 8
#define EXIT 98
#define HELP 99


struct row{
	char tmn[20],mv[20],name[40],num[10],district[10],lotdesc[30],mapno[20],db[10],dp[10],yd[10],td[15],acre[10],ratio[10],addr[100],prefaddr[200],pv[20],tv[20],lots[10],numbldg[10],
		landass[10],bldgass[10],totass[10],rc[10],paydate[20];
	bool img,ptimg;
};
std::vector<struct row> table;
char * bad,*reject;
int badsize,rsize;
int redoimg;

struct MemoryStruct {
	char *memory;
	size_t size;
};

int getTask();
int downloadTable();
void downloadImages();
void downloadPropertyDetails();
void saveTable();
void loadTable();
void importTaxMapList();
void exportTable();
string getRecommendations(MemoryStruct &, char *);
void paid();
void clearRow(row * r);


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
size_t write_image(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	//if(badsize==nmemb){
		int i,j=0,k=0;
		int n=min(badsize,rsize);
		n=min(n,(int)nmemb);
		for(i=0;i<n;i++){
			if(((char*)ptr)[i]!=bad[i])j++;
			if(((char*)ptr)[i]!=reject[i])k++;
		}
		if(((float)j)/n<0.05)return 0;
		if(((float)k)/n<0.05){
			if(redoimg==0)redoimg=10;
			else redoimg*=10;
			return 0;
		}
		redoimg=0;
	//}
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
void getData(MemoryStruct* chunk,const char * num){
	do{
		CURL *curl;
		CURLcode res;
		curl_global_init(CURL_GLOBAL_ALL);
		//struct curl_httppost *formpost=NULL;
		//struct curl_httppost *lastptr=NULL;
		struct curl_slist *headerlist=NULL;
		static const char buf[] = "Expect:";
		/* Fill in the filename field */
		/*curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "QryMapNo",
			CURLFORM_COPYCONTENTS, num,
			CURLFORM_END);*/


		/* Fill in the submit field too, even if this is rarely needed */
		/*curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "submit",
			CURLFORM_COPYCONTENTS, "send",
			CURLFORM_END);*/

		curl = curl_easy_init();
		/* initalize custom header list (stating that Expect: 100-continue is not
		wanted */
		headerlist = curl_slist_append(headerlist, buf);
		if(curl) {
			/* what URL that receives this POST */
			curl_easy_setopt(curl, CURLOPT_URL, "https://acpass.andersoncountysc.org/asrmain.cgi");
			/* send all data to this function  */
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			/* we pass our 'chunk' struct to the callback function */
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
			/* some servers don't like requests that are made without a user-agent
			field, so we provide one */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			//if ( (argc == 2) && (!strcmp(argv[1], "noexpectheader")) )
				/* only disable 100-continue header if explicitly requested */
			//		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
			//curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
			char data[100]="QryMapNo=";
			strcat(data,num);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			/* Check for errors */
			if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			curl_easy_reset(curl);
			/* always cleanup */
			curl_easy_cleanup(curl);

			/* then cleanup the formpost chain */
			//curl_formfree(formpost);
			/* free slist */
			curl_slist_free_all (headerlist);
		}
		//curl_easy_reset(curl);
		/*ofstream myfile;
          myfile.open ("log.txt");
          myfile << chunk->size << std::endl << chunk->memory <<std::endl;
          myfile.close();*/
	}while(chunk->memory==NULL);
}
char * getAppSession(){
	CURL *curl;
	CURLcode res;
	MemoryStruct  chunk;
	curl_global_init(CURL_GLOBAL_ALL);
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	char addr[300]="http://b2.caspio.com/dp.asp?appkey=60d21000c2a3a0h8g6i2f1b4a5f2";
	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, addr);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		curl_easy_reset(curl);
		curl_slist_free_all (headerlist);
	}
	char ret[20]={0};
	char * parser=strstr(chunk.memory,"appSession=");
	if(parser==NULL)printf("%s",chunk.memory);
	else{
		parser+=11;
		int i;
		for(i=0;parser[i]>='0'&&parser[i]<='9';i++)ret[i]=parser[i];
		ret[i]=0;
	}
	return ret;
}

void getIds(char mapno[12], char ids[3][14]){
	CURL *curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	char addr[69]="http://acpass.andersoncountysc.org/rpcmapnbrb.cgi?mapno=";
	MemoryStruct chunk;
	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	strcat(addr,mapno);
	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, addr);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		curl_easy_reset(curl);
		curl_slist_free_all (headerlist);
	}

	char * parser;
	memset(ids,0,sizeof(char)*3*14);
	parser=strstr(chunk.memory,"<input type=\"checkbox\" name=\"acctnbr\" ");
	if(parser!=NULL)do{
		strcpy(ids[2],ids[1]);
		strcpy(ids[1],ids[0]);
		for(int i=0;i<13;i++)ids[0][i]=*(parser+45+i);
		parser++;
	}while((parser=strstr(parser,"<input type=\"checkbox\" name=\"acctnbr\" "))!=NULL);

	free(chunk.memory);
}


string getRecommendations(MemoryStruct & chunk, char * address){
	CURL *curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	char * session=getAppSession();
	char addr[300]="http://maps.googleapis.com/maps/api/geocode/xml?bounds=34.187311,-83.001022|34.805276,-82.297897&sensor=false&address=";
	strcat(addr,address);
	string addrs(addr);
	int pos;
	while((pos=addrs.find(' '))!=string::npos){
		addrs.replace(addrs.begin()+pos,addrs.begin()+pos+1,"%20");
	}
	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, addrs.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		curl_easy_reset(curl);
		curl_slist_free_all (headerlist);
	}

	char * parser, * end, * ac;
	string ret;
	ac=strstr(chunk.memory,"<short_name>Anderson</short_name><type>administrative_area_level_2</type>");
	parser=strstr(chunk.memory,"<formatted_address>");
	if(parser==NULL)return string();
	if(ac!=NULL){
		end=strstr(parser+1,"<formatted_address>");
		while(end<ac&&end!=NULL){
			parser=end;
			end=strstr(end+1,"<formatted_address>");
		}
	}
	end = strstr(parser,"</formatted_address>");
	ret=string(parser+19);
	ret.resize(end-(parser+19));
	return ret;
}


char* loadPage(MemoryStruct & chunk,int page){
	CURL *curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	char * session=getAppSession();
	char addr[300]="http://b2.caspio.com/dp.asp?appSession=";
	strcat(addr,session);
	strcat(addr,"&RecordID=&PageID=2&PrevPageID=&cpipage=");
	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	char pagen[8];
	_itoa(page,pagen,10);
	strcat(addr,pagen);
	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, addr);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		curl_easy_reset(curl);
		curl_slist_free_all (headerlist);
	}
	char * ret=strstr(chunk.memory,"cbTable");
	return ret;
	//free(chunk.memory);
}

bool match(char * chunk, char * str, int len){
	for(int i=0;i <len;i++)if(chunk[i]!=str[i])return false;
	return true;
}


int main(int argc, char *argv[])
{
	int task;
	bool exit=false;
	redoimg=0;
    system("mode 150,50");
	while(exit==false){
		task = getTask();
		switch(task){
		case TASK_DL_TABLE: if(downloadTable()!=0)printf("ERROR\n");
		break; case TASK_DL_DETAILS: downloadPropertyDetails();
		break; case TASK_DL_IMAGES: downloadImages();
		break; case TASK_PAID: paid();
		break; case SAVE_TABLE: saveTable();
		break; case LOAD_TABLE: loadTable();
		break; case IMPORT_TAXMAP_LIST: importTaxMapList();
		break; case EXPORT_TABLE: exportTable();
		break; case EXIT: exit=true;
		break; case HELP: default:
			printf("Help Text\n");
		}
	}
}

int getTask(){
	int task=0;
	int tasks[]={
		TASK_DL_TABLE,
		TASK_DL_DETAILS,
		TASK_DL_IMAGES,
		TASK_PAID,
		SAVE_TABLE,
		LOAD_TABLE,
		IMPORT_TAXMAP_LIST,
		EXPORT_TABLE,
		EXIT,
		HELP};
	char tasktext[100][40];
	strcpy(tasktext[TASK_DL_TABLE],"Download the Table");
	strcpy(tasktext[TASK_DL_DETAILS],"Download the Details");
	strcpy(tasktext[TASK_DL_IMAGES],"Download the Images");
	strcpy(tasktext[TASK_PAID],"Download the pay status");
	strcpy(tasktext[SAVE_TABLE],"Save this Table");
	strcpy(tasktext[LOAD_TABLE],"Load Table");
	strcpy(tasktext[IMPORT_TAXMAP_LIST],"Import Taxmap Numbers From File");
	strcpy(tasktext[EXPORT_TABLE],"Export the table to an Excel File");
	strcpy(tasktext[HELP],"Help");
	strcpy(tasktext[EXIT],"Exit");

	while(task==0){
		printf("\nChoose a task by entering its number and pressing enter:\n");

		for(int i=0;i<sizeof(tasks)/sizeof(tasks[1]);i++){
			printf("%d. %s\n",tasks[i],tasktext[tasks[i]]);
		}
		scanf("%d",&task);

		for(int i=0;i<sizeof(tasks)/sizeof(tasks[1]);i++){
			if(task==tasks[i])return task;
		}
		task=0;
		printf("Incorrect Code\n");
	}
}



int downloadTable(){
	struct MemoryStruct chunk;

	/*FILE * in;
	fopen_s(&in,"mapnos.txt","r");
	if(in==NULL)return -3;*/

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	std::string header="Name\tNumber\tDistrict\tLot Description\tMap No.\tDeed Book\tDeed Page\tYear Due\tTax Due\tAcres\tRatio\n";


	int i=1,rownum=0;
	char * parser,*end;
	while(((parser=loadPage(chunk,i))!=NULL)&&i!=116){
		parser=strstr(parser,"</tr>");
		parser++;
		struct row srow={0}; int col=0;
		while(!match(parser,"</table>",8)){
			if(match(parser,"<td ",4)){
				parser++;
				parser=strstr(parser,">");
				parser++;
				end=strstr(parser,"</td>");
				//fwrite(parser,sizeof(char),end-parser,out);
				//fwrite("\t",sizeof(char),1,out);

				if(col==0)strncpy(srow.name,parser,end-parser);
				else if(col==1)strncpy(srow.num,parser,end-parser);
				else if(col==2)strncpy(srow.district,parser,end-parser);
				else if(col==3)strncpy(srow.lotdesc,parser,end-parser);
				else if(col==4)strncpy(srow.mapno,parser,end-parser);
				else if(col==5)strncpy(srow.db,parser,end-parser);
				else if(col==6)strncpy(srow.dp,parser,end-parser);
				else if(col==7)strncpy(srow.yd,parser,end-parser);
				else if(col==8)strncpy(srow.td,parser,end-parser);
				else if(col==9)strncpy(srow.acre,parser,end-parser);
				else if(col==10)strncpy(srow.ratio,parser,end-parser);
				col++;
			}
			else if(match(parser,"</tr>",5)){
				rownum++;col=0;
				table.push_back(srow);
			}
			parser++;
		}

		printf("Downloaded page %d :: Records %d-%d\n",i,(i-1)*25+1,i*25);
		i++;
		free(chunk.memory);
		memset(&chunk,0,sizeof(MemoryStruct));
	}


	//system("pause");
	printf("The table has been stored in memory\n");
	free(chunk.memory);
	return 0;
}
void downloadImages(){
	//GetFileAttributes("workbook\\xl\\media\\");
	_wmkdir(L"workbook\\xl\\media\\");
	FILE * badin;
	badin = fopen("noImage.jpg","rb");
	fseek(badin,0,SEEK_END);
	badsize=ftell(badin);
	fseek(badin,0,SEEK_SET);
	bad=(char*)malloc(badsize);
	fread(bad,1,badsize,badin);
	fclose(badin);
	badin=fopen("rejectImage.jpg","rb");
	fseek(badin,0,SEEK_END);
	rsize=ftell(badin);
	fseek(badin,0,SEEK_SET);
	reject=(char*)malloc(rsize);
	fread(reject,1,rsize,badin);
	fclose(badin);

	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	struct row ** redos=(struct row **)malloc(table.size()*sizeof(struct row *));
	memset(redos,0,table.size()*sizeof(struct row *));
	int redoit=0;
	int pref=0,npref=0,nex=0;
	for(int i=0;i<table.size();i++){
		//do{
		//if(i<700)continue;
		string tempfn(table[i].mapno);
		tempfn="workbook\\xl\\media\\"+tempfn+".jpg";
		ifstream ifile(tempfn.c_str());
		if (ifile) {
			continue;
		}
		ifile.close();
		//if(i>0 && i%500==0)printf("\t%d %d %d",pref,npref,nex);
		//if(i%500==0)printf("\nTotal: ");
		//if(i>0 && i%50==0)printf(" %d",i);

			if(strchr(table[i].addr,'&')!=NULL)continue;

			string prefaddr=getRecommendations(chunk,table[i].addr);

			CURL *curl;
			FILE *fp=NULL;
			CURLcode res;
			char url[400] = "http://maps.googleapis.com/maps/api/streetview?size=400x400&key=redacted&fov=90&sensor=false&location=";
			if(prefaddr.length()!=0){
				strcat(url,prefaddr.c_str());
				pref++;
				strcpy(table[i].prefaddr,prefaddr.c_str());
			}
			else{
				printf("%d: No preferred found: %s %s\n",i,table[i].mapno,table[i].addr);
				npref++;
				strcat(url,table[i].addr);
				strcat(url,", SC");
			}
			char outfilename[FILENAME_MAX] = "workbook\\xl\\media\\";
			strcat(outfilename,table[i].mapno);
			strcat(outfilename,".jpg");
			int j=0;
			while(table[i].mapno[j]!=0){
				if(table[i].mapno[j]<'0'||table[i].mapno[j]>'9')break;
				j++;
			}
			if(table[i].mapno[j]!=0)continue;
			j=0;
			while(j<399&&url[j]!=0){
				if(url[j]==' '){
					for(int k=199;k>j+2;k--)url[k]=url[k-2];
					url[j]='%';
					url[j+1]='2';
					url[j+2]='0';
				}
				j++;
			}

			curl = curl_easy_init();
			if (curl) {
				fp=fopen(outfilename,"wb");
				if(fp==NULL){
					i--;
					continue;
				}
				curl_easy_setopt(curl, CURLOPT_URL, url);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_image);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
				char * test=table[i].mapno;
				res = curl_easy_perform(curl);
				/* always cleanup */
				curl_easy_cleanup(curl);
				bool del=false;int moved=ftell(fp);
				if(moved==0)del=true;
				fclose(fp);
				if(del){
					remove(outfilename);
					nex++;
					printf("%d: No image found: %s %s::%s\n",i,table[i].mapno,table[i].addr,prefaddr.c_str());
					table[i].img=false;
				}
				else table[i].img=true;
			}
			//Sleep(redoimg);
			if(redoimg){
				redos[redoit]=&table[i];
				redoit++;
			}
		//}while(redoimg);
	}
	printf("\n");
	free(bad);
}

void downloadPropertyDetails(){
	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */



	/* Fill in the file upload field */
	/*curl_formadd(&formpost,
	&lastptr,
	CURLFORM_COPYNAME, "sendfile",
	CURLFORM_FILE, "postit2.c",
	CURLFORM_END);*/
	for(int ml=0;ml<table.size();ml++){
            //std::cout << table[ml].mapno <<std::endl;
		getData(&chunk,table[ml].mapno);
		//printf("%s\n",chunk.memory);
		//printf("/////////////////////\n\
		/////////////////////\n");
            char * parser = strstr(chunk.memory,"<table");
            char * end;
            if(parser==NULL){
                table[ml].tmn[0]=0;
                table[ml].mv[0]=0;
                table[ml].tv[0]=0;
                table[ml].addr[0]=0;
                table[ml].lots[0]=0;
                table[ml].numbldg[0]=0;
                table[ml].bldgass[0]=0;
                table[ml].totass[0]=0;
                table[ml].rc[0]=0;

                continue;
            }
            parser=strstr(parser,"<font color=\"#FF0000\"><strong>");
            if(parser != NULL){
                parser+=30;
                end=strstr(parser,"</strong>");
                strncpy(table[ml].tmn,parser,end-parser);
                parser=strstr(parser,"Physical\n                  Address");
                parser=strstr(parser,"<td>")+4;
                parser=strstr(parser,">")+1;
                end=strstr(parser,"</font>");
                strncpy(table[ml].addr,parser,end-parser);
                parser=strstr(strstr(parser,"Market Value"),"<td>")+4;
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy(table[ml].mv,parser,end-parser);
                parser=strstr(strstr(parser,"Prior Value"),"<td>")+4;
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy(table[ml].pv,parser,end-parser);
                parser=strstr(strstr(parser,"Tax Value"),"<td>")+4;
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy(table[ml].tv,parser,end-parser);
                parser=strstr(strstr(strstr(strstr(strstr(strstr(parser,"YEAR"),"ACRES"),"LOTS"),"<tr>"),"<td")+1,"<td")+1;

                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].acre,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].lots,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].landass,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].numbldg,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].bldgass,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].totass,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;
                end=strstr(parser,"</"); parser=end;
                while(parser[0]!='>')parser--; parser++;
                strncpy(table[ml].rc,parser,end-parser);
                parser=strstr(parser,"<td"); end=parser;

                /*while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].lots,sizeof(table[ml].lots),parser,end-parser);
                parser=strstr(parser,"<td");
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].landass,sizeof(table[ml].landass),parser,end-parser);
                parser=strstr(parser,"<td");
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].numbldg,sizeof(table[ml].numbldg),parser,end-parser);
                parser=strstr(parser,"<td");
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].bldgass,sizeof(table[ml].bldgass),parser,end-parser);
                parser=strstr(parser,"<td");
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].totass,sizeof(table[ml].totass),parser,end-parser);
                parser=strstr(parser,"<td");
                while(parser[0]=='<')parser=strstr(parser,">")+1;
                end=strstr(parser,"<");
                strncpy_s(table[ml].rc,sizeof(table[ml].rc),parser,end-parser);*/
            }


            if(ml%25==24){
                printf("Completed %d to %d of %d entries\n",ml-24,ml+1,table.size());
            }
            memset(&chunk,0,sizeof(MemoryStruct));
	}
	//printf("%s",parser);
	//system("pause");
	free(chunk.memory);

}

void saveTable(){
	FILE * incheck;
	char ans=0;
	incheck=fopen("table.save","rb");
	if(incheck!=NULL){
		fclose(incheck);
		printf("There is already a table saved. Are you sure you want to overwrite it?\n(y/n): ");
		while(ans!='y'&&ans!='n')scanf("%c",&ans);
		if(ans=='n')return;
	}
	if(table.size()==0){
		printf("The table you are saving is empty. Are you sure you want to save it?\n(y/n): ");
		while(ans!='y'&&ans!='n')scanf("%c",&ans);
		if(ans=='n')return;
	}

	FILE * out;
	out=fopen("table.save","wb");
	int size = table.size();
	fwrite((void *)&size,sizeof(int),1,out);
	for(int i=0;i<table.size();i++)fwrite((void *)&table[i],sizeof(struct row),1,out);



	fclose(out);
}

void loadTable(){
	FILE * in;
	in=fopen("table.save","rb");
	table.clear();
	int size;
	fread(&size,sizeof(int),1,in);
	for(int i=0;i<size;i++){
		struct row trow={0};
		fread(&trow,sizeof(struct row),1,in);
		table.push_back(trow);

	}
	fclose(in);
}

std::string to_utf8(const wchar_t* buffer, int len)
{
   int nChars = ::WideCharToMultiByte(
      CP_UTF8,
      0,
      buffer,
      len,
      NULL,
      0,
      NULL,
      NULL);
   if (nChars == 0) return "";

   string newbuffer;
   newbuffer.resize(nChars) ;
   ::WideCharToMultiByte(
      CP_UTF8,
      0,
      buffer,
      len,
      const_cast< char* >(newbuffer.c_str()),
      nChars,
      NULL,
      NULL);

   return newbuffer;
}

std::string to_utf8(const std::wstring& str)
{
   return to_utf8(str.c_str(), (int)str.size());
}

void exportTable(){
	//if(table.size()>0){
		std::wstring content_types_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/><Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/><Override PartName=\"/xl/worksheets/sheet2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/><Override PartName=\"/xl/worksheets/sheet3.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/><Override PartName=\"/xl/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/><Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/><Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/><Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/></Types>";
		std::wstring root_rels=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/></Relationships>";
		std::wstring workbook_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><fileVersion appName=\"xl\" lastEdited=\"5\" lowestEdited=\"5\" rupBuild=\"9303\"/><workbookPr defaultThemeVersion=\"124226\"/><bookViews><workbookView xWindow=\"120\" yWindow=\"60\" windowWidth=\"19155\" windowHeight=\"7230\"/></bookViews><sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\"/><sheet name=\"Sheet2\" sheetId=\"2\" r:id=\"rId2\"/><sheet name=\"Sheet3\" sheetId=\"3\" r:id=\"rId3\"/></sheets><calcPr calcId=\"145621\"/></workbook>";
		std::wstring workbook_rels=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet3.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet2.xml\"/><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/><Relationship Id=\"rId5\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/><Relationship Id=\"rId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"theme/theme1.xml\"/></Relationships>";
		std::wstring worksheet2_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\" xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\"><dimension ref=\"A1\"/><sheetViews><sheetView workbookViewId=\"0\"/></sheetViews><sheetFormatPr defaultRowHeight=\"15\" x14ac:dyDescent=\"0.25\"/><sheetData/><pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" header=\"0.3\" footer=\"0.3\"/></worksheet>";
		std::wstring worksheet3_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\" xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\"><dimension ref=\"A1\"/><sheetViews><sheetView workbookViewId=\"0\"/></sheetViews><sheetFormatPr defaultRowHeight=\"15\" x14ac:dyDescent=\"0.25\"/><sheetData/><pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" header=\"0.3\" footer=\"0.3\"/></worksheet>";
		std::wstring theme_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Office Theme\"><a:themeElements><a:clrScheme name=\"Office\"><a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1><a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1><a:dk2><a:srgbClr val=\"1F497D\"/></a:dk2><a:lt2><a:srgbClr val=\"EEECE1\"/></a:lt2><a:accent1><a:srgbClr val=\"4F81BD\"/></a:accent1><a:accent2><a:srgbClr val=\"C0504D\"/></a:accent2><a:accent3><a:srgbClr val=\"9BBB59\"/></a:accent3><a:accent4><a:srgbClr val=\"8064A2\"/></a:accent4><a:accent5><a:srgbClr val=\"4BACC6\"/></a:accent5><a:accent6><a:srgbClr val=\"F79646\"/></a:accent6><a:hlink><a:srgbClr val=\"0000FF\"/></a:hlink><a:folHlink><a:srgbClr val=\"800080\"/></a:folHlink></a:clrScheme><a:fontScheme name=\"Office\"><a:majorFont><a:latin typeface=\"Cambria\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/><a:font script=\"Jpan\" typeface=\"ＭＳ Ｐゴシック\"/><a:font script=\"Hang\" typeface=\"맑은 고딕\"/><a:font script=\"Hans\" typeface=\"宋体\"/><a:font script=\"Hant\" typeface=\"新細明體\"/><a:font script=\"Arab\" typeface=\"Times New Roman\"/><a:font script=\"Hebr\" typeface=\"Times New Roman\"/><a:font script=\"Thai\" typeface=\"Tahoma\"/><a:font script=\"Ethi\" typeface=\"Nyala\"/><a:font script=\"Beng\" typeface=\"Vrinda\"/><a:font script=\"Gujr\" typeface=\"Shruti\"/><a:font script=\"Khmr\" typeface=\"MoolBoran\"/><a:font script=\"Knda\" typeface=\"Tunga\"/><a:font script=\"Guru\" typeface=\"Raavi\"/><a:font script=\"Cans\" typeface=\"Euphemia\"/><a:font script=\"Cher\" typeface=\"Plantagenet Cherokee\"/><a:font script=\"Yiii\" typeface=\"Microsoft Yi Baiti\"/><a:font script=\"Tibt\" typeface=\"Microsoft Himalaya\"/><a:font script=\"Thaa\" typeface=\"MV Boli\"/><a:font script=\"Deva\" typeface=\"Mangal\"/><a:font script=\"Telu\" typeface=\"Gautami\"/><a:font script=\"Taml\" typeface=\"Latha\"/><a:font script=\"Syrc\" typeface=\"Estrangelo Edessa\"/><a:font script=\"Orya\" typeface=\"Kalinga\"/><a:font script=\"Mlym\" typeface=\"Kartika\"/><a:font script=\"Laoo\" typeface=\"DokChampa\"/><a:font script=\"Sinh\" typeface=\"Iskoola Pota\"/><a:font script=\"Mong\" typeface=\"Mongolian Baiti\"/><a:font script=\"Viet\" typeface=\"Times New Roman\"/><a:font script=\"Uigh\" typeface=\"Microsoft Uighur\"/><a:font script=\"Geor\" typeface=\"Sylfaen\"/></a:majorFont><a:minorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/><a:font script=\"Jpan\" typeface=\"ＭＳ Ｐゴシック\"/><a:font script=\"Hang\" typeface=\"맑은 고딕\"/><a:font script=\"Hans\" typeface=\"宋体\"/><a:font script=\"Hant\" typeface=\"新細明體\"/><a:font script=\"Arab\" typeface=\"Arial\"/><a:font script=\"Hebr\" typeface=\"Arial\"/><a:font script=\"Thai\" typeface=\"Tahoma\"/><a:font script=\"Ethi\" typeface=\"Nyala\"/><a:font script=\"Beng\" typeface=\"Vrinda\"/><a:font script=\"Gujr\" typeface=\"Shruti\"/><a:font script=\"Khmr\" typeface=\"DaunPenh\"/><a:font script=\"Knda\" typeface=\"Tunga\"/><a:font script=\"Guru\" typeface=\"Raavi\"/><a:font script=\"Cans\" typeface=\"Euphemia\"/><a:font script=\"Cher\" typeface=\"Plantagenet Cherokee\"/><a:font script=\"Yiii\" typeface=\"Microsoft Yi Baiti\"/><a:font script=\"Tibt\" typeface=\"Microsoft Himalaya\"/><a:font script=\"Thaa\" typeface=\"MV Boli\"/><a:font script=\"Deva\" typeface=\"Mangal\"/><a:font script=\"Telu\" typeface=\"Gautami\"/><a:font script=\"Taml\" typeface=\"Latha\"/><a:font script=\"Syrc\" typeface=\"Estrangelo Edessa\"/><a:font script=\"Orya\" typeface=\"Kalinga\"/><a:font script=\"Mlym\" typeface=\"Kartika\"/><a:font script=\"Laoo\" typeface=\"DokChampa\"/><a:font script=\"Sinh\" typeface=\"Iskoola Pota\"/><a:font script=\"Mong\" typeface=\"Mongolian Baiti\"/><a:font script=\"Viet\" typeface=\"Arial\"/><a:font script=\"Uigh\" typeface=\"Microsoft Uighur\"/><a:font script=\"Geor\" typeface=\"Sylfaen\"/></a:minorFont></a:fontScheme><a:fmtScheme name=\"Office\"><a:fillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:tint val=\"50000\"/><a:satMod val=\"300000\"/></a:schemeClr></a:gs><a:gs pos=\"35000\"><a:schemeClr val=\"phClr\"><a:tint val=\"37000\"/><a:satMod val=\"300000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:tint val=\"15000\"/><a:satMod val=\"350000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"16200000\" scaled=\"1\"/></a:gradFill><a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:shade val=\"51000\"/><a:satMod val=\"130000\"/></a:schemeClr></a:gs><a:gs pos=\"80000\"><a:schemeClr val=\"phClr\"><a:shade val=\"93000\"/><a:satMod val=\"130000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:shade val=\"94000\"/><a:satMod val=\"135000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"16200000\" scaled=\"0\"/></a:gradFill></a:fillStyleLst><a:lnStyleLst><a:ln w=\"9525\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"><a:shade val=\"95000\"/><a:satMod val=\"105000\"/></a:schemeClr></a:solidFill><a:prstDash val=\"solid\"/></a:ln><a:ln w=\"25400\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln><a:ln w=\"38100\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst><a:outerShdw blurRad=\"40000\" dist=\"20000\" dir=\"5400000\" rotWithShape=\"0\"><a:srgbClr val=\"000000\"><a:alpha val=\"38000\"/></a:srgbClr></a:outerShdw></a:effectLst></a:effectStyle><a:effectStyle><a:effectLst><a:outerShdw blurRad=\"40000\" dist=\"23000\" dir=\"5400000\" rotWithShape=\"0\"><a:srgbClr val=\"000000\"><a:alpha val=\"35000\"/></a:srgbClr></a:outerShdw></a:effectLst></a:effectStyle><a:effectStyle><a:effectLst><a:outerShdw blurRad=\"40000\" dist=\"23000\" dir=\"5400000\" rotWithShape=\"0\"><a:srgbClr val=\"000000\"><a:alpha val=\"35000\"/></a:srgbClr></a:outerShdw></a:effectLst><a:scene3d><a:camera prst=\"orthographicFront\"><a:rot lat=\"0\" lon=\"0\" rev=\"0\"/></a:camera><a:lightRig rig=\"threePt\" dir=\"t\"><a:rot lat=\"0\" lon=\"0\" rev=\"1200000\"/></a:lightRig></a:scene3d><a:sp3d><a:bevelT w=\"63500\" h=\"25400\"/></a:sp3d></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:tint val=\"40000\"/><a:satMod val=\"350000\"/></a:schemeClr></a:gs><a:gs pos=\"40000\"><a:schemeClr val=\"phClr\"><a:tint val=\"45000\"/><a:shade val=\"99000\"/><a:satMod val=\"350000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:shade val=\"20000\"/><a:satMod val=\"255000\"/></a:schemeClr></a:gs></a:gsLst><a:path path=\"circle\"><a:fillToRect l=\"50000\" t=\"-80000\" r=\"50000\" b=\"180000\"/></a:path></a:gradFill><a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:tint val=\"80000\"/><a:satMod val=\"300000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:shade val=\"30000\"/><a:satMod val=\"200000\"/></a:schemeClr></a:gs></a:gsLst><a:path path=\"circle\"><a:fillToRect l=\"50000\" t=\"50000\" r=\"50000\" b=\"50000\"/></a:path></a:gradFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>";
		std::wstring styles_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\" xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\"><fonts count=\"1\" x14ac:knownFonts=\"1\"><font><sz val=\"11\"/><color theme=\"1\"/><name val=\"Calibri\"/><family val=\"2\"/><scheme val=\"minor\"/></font></fonts><fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill><fill><patternFill patternType=\"gray125\"/></fill></fills><borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders><cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs><cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs><cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles><dxfs count=\"0\"/><tableStyles count=\"0\" defaultTableStyle=\"TableStyleMedium2\" defaultPivotStyle=\"PivotStyleLight16\"/><extLst><ext uri=\"{EB79DEF2-80B8-43e5-95BD-54CBDDF9020C}\" xmlns:x14=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/main\"><x14:slicerStyles defaultSlicerStyle=\"SlicerStyleLight1\"/></ext></extLst></styleSheet>";
		std::wstring app_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\"><Application>Microsoft Excel</Application><DocSecurity>0</DocSecurity><ScaleCrop>false</ScaleCrop><HeadingPairs><vt:vector size=\"2\" baseType=\"variant\"><vt:variant><vt:lpstr>Worksheets</vt:lpstr></vt:variant><vt:variant><vt:i4>3</vt:i4></vt:variant></vt:vector></HeadingPairs><TitlesOfParts><vt:vector size=\"3\" baseType=\"lpstr\"><vt:lpstr>Sheet1</vt:lpstr><vt:lpstr>Sheet2</vt:lpstr><vt:lpstr>Sheet3</vt:lpstr></vt:vector></TitlesOfParts><Company>Clemson University</Company><LinksUpToDate>false</LinksUpToDate><SharedDoc>false</SharedDoc><HyperlinksChanged>false</HyperlinksChanged><AppVersion>14.0300</AppVersion></Properties>";
		std::wstring core_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><dc:creator>Arthur</dc:creator><cp:lastModifiedBy>Arthur</cp:lastModifiedBy><dcterms:created xsi:type=\"dcterms:W3CDTF\">2013-09-21T13:23:02Z</dcterms:created><dcterms:modified xsi:type=\"dcterms:W3CDTF\">2013-09-21T13:23:50Z</dcterms:modified></cp:coreProperties>";

		//std::wstring worksheet_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\" xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\"><dimension ref=\"A1\"/><sheetViews><sheetView tabSelected=\"1\" workbookViewId=\"0\"/></sheetViews><sheetFormatPr defaultRowHeight=\"15\" x14ac:dyDescent=\"0.25\"/><sheetData/><pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" header=\"0.3\" footer=\"0.3\"/></worksheet>";
		std::wstring worksheet_xml=L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\" xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\"><dimension ref=\"A1\"/><sheetViews><sheetView tabSelected=\"1\" workbookViewId=\"0\"/></sheetViews><sheetFormatPr defaultRowHeight=\"15\" x14ac:dyDescent=\"0.25\"/><sheetData>";
		std::wstringstream  wss;


		wss << L"<row r=\"" << 1 << L"\" spans=\"1:26\">"; //23
			wss << L"<c r=\"A"<<1<<L"\" t=\"inlineStr\"><is><t>Map Number</t></is></c>";
			wss << L"<c r=\"B"<<1<<L"\" t=\"inlineStr\"><is><t>Map Number Formatted</t></is></c>";
			wss << L"<c r=\"C"<<1<<L"\" t=\"inlineStr\"><is><t>Market Value</t></is></c>";
			wss << L"<c r=\"D"<<1<<L"\" t=\"inlineStr\"><is><t>Owner Name</t></is></c>";
			wss << L"<c r=\"E"<<1<<L"\" t=\"inlineStr\"><is><t>Number</t></is></c>";
			wss << L"<c r=\"F"<<1<<L"\" t=\"inlineStr\"><is><t>District</t></is></c>";
			wss << L"<c r=\"G"<<1<<L"\" t=\"inlineStr\"><is><t>Lot Desc</t></is></c>";
			wss << L"<c r=\"H"<<1<<L"\" t=\"inlineStr\"><is><t>Deed Book</t></is></c>";
			wss << L"<c r=\"I"<<1<<L"\" t=\"inlineStr\"><is><t>Deed Page</t></is></c>";
			wss << L"<c r=\"J"<<1<<L"\" t=\"inlineStr\"><is><t>Year Due</t></is></c>";
			wss << L"<c r=\"K"<<1<<L"\" t=\"inlineStr\"><is><t>Tax Due</t></is></c>";
			wss << L"<c r=\"L"<<1<<L"\" t=\"inlineStr\"><is><t>Acres</t></is></c>";
			wss << L"<c r=\"M"<<1<<L"\" t=\"inlineStr\"><is><t>Ratio</t></is></c>";
			wss << L"<c r=\"N"<<1<<L"\" t=\"inlineStr\"><is><t>Address</t></is></c>";
			wss << L"<c r=\"O"<<1<<L"\" t=\"inlineStr\"><is><t>Recommended Address</t></is></c>";
			wss << L"<c r=\"P"<<1<<L"\" t=\"inlineStr\"><is><t>Prior Value</t></is></c>";
			wss << L"<c r=\"Q"<<1<<L"\" t=\"inlineStr\"><is><t>Tax Value</t></is></c>";
			wss << L"<c r=\"R"<<1<<L"\" t=\"inlineStr\"><is><t>Lots</t></is></c>";
			wss << L"<c r=\"S"<<1<<L"\" t=\"inlineStr\"><is><t>Num Buildings</t></is></c>";
			wss << L"<c r=\"T"<<1<<L"\" t=\"inlineStr\"><is><t>Land Asmt</t></is></c>";
			wss << L"<c r=\"U"<<1<<L"\" t=\"inlineStr\"><is><t>Bldg Asmt</t></is></c>";
			wss << L"<c r=\"V"<<1<<L"\" t=\"inlineStr\"><is><t>Tot Asmt</t></is></c>";
			wss << L"<c r=\"W"<<1<<L"\" t=\"inlineStr\"><is><t>RC</t></is></c>";
			wss << L"<c r=\"X"<<1<<L"\" t=\"inlineStr\"><is><t>Img Available</t></is></c>";
			wss << L"<c r=\"Y"<<1<<L"\" t=\"inlineStr\"><is><t>Pay Img</t></is></c>";
			wss << L"<c r=\"Z"<<1<<L"\" t=\"inlineStr\"><is><t>Pay Date</t></is></c>";
		wss << L"</row>\n";
		for(int i = 0; i<table.size();i++){
			wss << L"<row r=\"" << i+2 << L"\" spans=\"1:26\">"; //23

			wss << L"<c r=\"A"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].mapno <<L"</t></is></c>";
			wss << L"<c r=\"B"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].tmn <<L"</t></is></c>";
			wss << L"<c r=\"C"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].mv <<L"</t></is></c>";
			wss << L"<c r=\"D"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].name <<L"</t></is></c>";
			wss << L"<c r=\"E"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].num <<L"</t></is></c>";
			wss << L"<c r=\"F"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].district <<L"</t></is></c>";
			wss << L"<c r=\"G"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].lotdesc <<L"</t></is></c>";
			wss << L"<c r=\"H"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].db <<L"</t></is></c>";
			wss << L"<c r=\"I"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].dp <<L"</t></is></c>";
			wss << L"<c r=\"J"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].yd <<L"</t></is></c>";
			wss << L"<c r=\"K"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].td <<L"</t></is></c>";
			wss << L"<c r=\"L"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].acre <<L"</t></is></c>";
			wss << L"<c r=\"M"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].ratio <<L"</t></is></c>";
			wss << L"<c r=\"N"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].addr <<L"</t></is></c>";
			wss << L"<c r=\"O"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].prefaddr <<L"</t></is></c>";
			wss << L"<c r=\"P"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].pv <<L"</t></is></c>";
			wss << L"<c r=\"Q"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].tv <<L"</t></is></c>";
			wss << L"<c r=\"R"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].lots <<L"</t></is></c>";
			wss << L"<c r=\"S"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].numbldg <<L"</t></is></c>";
			wss << L"<c r=\"T"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].landass <<L"</t></is></c>";
			wss << L"<c r=\"U"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].bldgass <<L"</t></is></c>";
			wss << L"<c r=\"V"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].totass <<L"</t></is></c>";
			wss << L"<c r=\"W"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].rc <<L"</t></is></c>";
			wss << L"<c r=\"X"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< ((table[i].img)?(L"Yes"):(L"NO")) <<L"</t></is></c>";
			wss << L"<c r=\"Y"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< ((table[i].ptimg)?(L"Yes"):(L"NO")) <<L"</t></is></c>";
			wss << L"<c r=\"Z"<<i+2<<L"\" t=\"inlineStr\"><is><t>"<< table[i].paydate <<L"</t></is></c>";

			wss << L"</row>\n";
		}
		worksheet_xml.append(wss.str());

		worksheet_xml.append(L"</sheetData><pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" header=\"0.3\" footer=\"0.3\"/></worksheet>");

		int match,per=0;
		while((match=worksheet_xml.find(L"&nbsp;",0,6))!=-1){
			worksheet_xml.replace(match,6,L" ",0,1);
			if(100*match/(float)(worksheet_xml.size())>per*2+10)printf("%d%% ",per+=5);
		}
		while((match=worksheet_xml.find(L"&",0,1))!=-1){
			worksheet_xml.replace(match,1,L"?",0,1);
			if(100*match/(float)(worksheet_xml.size())>(per-50)*2+10)printf("%d%% ",per+=5);
		}
		printf("100%%\n");

		/*char tmn[20],mv[20],name[40],num[10],district[10],lotdesc[30],mapno[20],db[10],dp[10],yd[10],td[15],acre[10],ratio[10],addr[100],pv[20],tv[20],lots[5],numbldg[5],
		landass[10],bldgass[10],totass[10],rc[10];
		bool img;*/

		_wmkdir(L"workbook\\");
		_wmkdir(L"workbook\\xl\\");
		_wmkdir(L"workbook\\_rels\\");
		_wmkdir(L"workbook\\docProps\\");
		_wmkdir(L"workbook\\xl\\_rels\\");
		_wmkdir(L"workbook\\xl\\theme\\");
		_wmkdir(L"workbook\\xl\\worksheets\\");
		_wmkdir(L"workbook\\xl\\media\\");

		std::ofstream ofs;
		ofs.open("workbook\\docProps\\core.xml",std::ofstream::out);
		ofs << to_utf8(core_xml).c_str();   ofs.close();
		ofs.open("workbook\\docProps\\app.xml",std::ofstream::out);
		ofs << to_utf8(app_xml).c_str();   ofs.close();
		ofs.open("workbook\\[Content_Types].xml",std::ofstream::out);
		ofs << to_utf8(content_types_xml).c_str();   ofs.close();
		ofs.open("workbook\\_rels\\.rels",std::ofstream::out);
		ofs << to_utf8(root_rels).c_str();   ofs.close();
		ofs.open("workbook\\xl\\workbook.xml",std::ofstream::out);
		ofs << to_utf8(workbook_xml).c_str();   ofs.close();
		ofs.open("workbook\\xl\\styles.xml",std::ofstream::out);
		ofs << to_utf8(styles_xml).c_str();   ofs.close();
		ofs.open("workbook\\xl\\_rels\\workbook.xml.rels",std::ofstream::out);
		ofs << to_utf8(workbook_rels).c_str();   ofs.close();
		ofs.open("workbook\\xl\\worksheets\\sheet1.xml",std::ofstream::out);
		ofs << to_utf8(worksheet_xml).c_str();   ofs.close();
		ofs.open("workbook\\xl\\worksheets\\sheet2.xml",std::ofstream::out);
		ofs << to_utf8(worksheet2_xml).c_str();   ofs.close();
		ofs.open("workbook\\xl\\worksheets\\sheet3.xml",std::ofstream::out);
		ofs << to_utf8(worksheet3_xml).c_str();   ofs.close();
		ofs.open("workbook\\xl\\theme\\theme1.xml",std::ofstream::out);
		ofs << to_utf8(theme_xml).c_str();   ofs.close();

/*
		FILE * out;
		fopen_s(&out,"workbook\\[Content_Types].xml","w");
		if(out!=NULL)fwrite(content_types_xml,1,sizeof(content_types_xml),out);
		fclose(out);
		fopen_s(&out,"workbook\\_rels\\.rels","w");
		if(out!=NULL)fwrite(root_rels,1,sizeof(root_rels),out);
		fclose(out);
		fopen_s(&out,"workbook\\xl\\workbook.xml","w");
		if(out!=NULL)fwrite(workbook_xml,1,sizeof(workbook_xml),out);
		fclose(out);
		fopen_s(&out,"workbook\\xl\\_rels\\workbook.xml.rels","w");
		if(out!=NULL)fwrite(workbook_rels,1,sizeof(workbook_rels),out);
		fclose(out);
		fopen_s(&out,"workbook\\xl\\worksheets\\sheet1.xml","w");
		if(out!=NULL)fwrite(worksheet_xml,1,sizeof(worksheet_xml),out);
		fclose(out);


		fopen_s(&out,"workbook\\docProps\\app.xml","w");
		if(out!=NULL)fwrite(app_xml,1,sizeof(app_xml),out);
		fclose(out);
		fopen_s(&out,"workbook\\docProps\\core.xml","w");
		if(out!=NULL)fwrite(core_xml,1,sizeof(core_xml),out);
		fclose(out);
		fopen_s(&out,"workbook\\xl\\styles.xml","w");
		if(out!=NULL)fwrite(styles_xml,1,sizeof(styles_xml),out);
		fclose(out);
		/*fopen_s(&out,"workbook\\xl\\theme\\theme1.xml","w,ccs=UTF-8");
		if(out!=NULL)fwrite(theme_xml,sizeof(wchar_t),sizeof(theme_xml),out);
		fclose(out);*/
		/*fopen_s(&out,"workbook\\xl\\worksheets\\sheet2.xml","w");
		if(out!=NULL)fwrite(worksheet2_xml,1,sizeof(worksheet2_xml),out);
		fclose(out);
		fopen_s(&out,"workbook\\xl\\worksheets\\sheet3.xml","w");
		if(out!=NULL)fwrite(worksheet3_xml,1,sizeof(worksheet3_xml),out);
		fclose(out);*/
	//}

	//Zipping

	//printf("Using zlib version %s\n",zlibVersion());
	system("\"C:/Program Files/7-Zip/7z\" d -tzip book.xlsx * -r ");
	system("\"C:/Program Files/7-Zip/7z\" a -tzip book.xlsx ./workbook/* ");
}

void importTaxMapList(){
	printf("Enter the filename: ");
	char fn[30]={0};
	scanf("%29s",fn);
	//int i;
	//scanf_s("%d",&i);
	filebuf fb;
	if(fb.open(fn,std::ios::in)){
		istream is(&fb);
		int num=0;
		while(is){
			row r={0};
			//clearRow(&r);
			is.getline(r.mapno,19,'\n');
			if(atoi(r.mapno)==0)continue;
			table.push_back(r);
			num++;
		}
		printf("%d map numbers imported.\n",num);
		fb.close();
		return;
	}
	printf("Could not open file\n");

}

void paid(){


	printf("Table size: %d\n",table.size());
	for(int i=0;i<table.size();i++){
		CURL *curl;
		CURLcode res;
		curl_global_init(CURL_GLOBAL_ALL);
		struct curl_slist *headerlist=NULL;
		static const char buf[] = "Expect:";
		char addr[69]="https://acpass.andersoncountysc.org/rpcmapnbrb.cgi?mapno=";
		MemoryStruct chunk;
		chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
		chunk.size = 0;    /* no data at this point */
		int mapnolen=strlen(table[i].mapno);
		char mapnoptr[15];
		for(int j=0;j<14;j++){
            if(table[i].mapno[j]<'0'||table[i].mapno[j]>'9'){
                mapnoptr[j]=0;
                break;
            }
            mapnoptr[j]=table[i].mapno[j];
		}
		for(int j=0;j<11-strlen(mapnoptr);j++){
            strcat(addr,"0");
		}
		strcat(addr,table[i].mapno);
		curl = curl_easy_init();
		headerlist = curl_slist_append(headerlist, buf);
		printf("Accessing %s\n",addr);
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, addr);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
			res = curl_easy_perform(curl);
			while(res != CURLE_OK){
				fprintf(stderr, "mapno:%s::curl_easy_perform() failed: %s\n",table[i].mapno,curl_easy_strerror(res));
				res = curl_easy_perform(curl);
			}
			curl_easy_reset(curl);
			curl_slist_free_all (headerlist);

            /*std::cout << "Comment this in paid()"<<std::endl;
            ofstream myfile;
              myfile.open ("log.txt");
              myfile << chunk.size << std::endl << chunk.memory <<std::endl;
              myfile.close();*/

			/*** PARSE ***/
			char * parser, *end, *secondLast;
			string purl;
			parser=strstr(chunk.memory,"<a href=\"rpcdetail1.cgi?acctnbr=");
			if(parser==NULL){
				if((parser=strstr(chunk.memory,"500 Internal Server Error"))!=NULL){Sleep(2000);i--;}
				else if((parser=strstr(chunk.memory,"Paid Date"))==NULL){
					printf("No pay infor for %s\n",table[i].mapno);
					std::ofstream ofs ("paiderr.txt", std::ofstream::out);
					ofs << chunk.memory;
					ofs.close();
                }
				else{
					parser=strstr(chunk.memory,"ptaxesno.gif");
					if(parser==NULL)table[i].ptimg=false;
					else table[i].ptimg=true;
					parser=strstr(chunk.memory,"Paid Date");
					parser=strstr(parser,"<strong>")+8;
					end=strstr(parser,"</strong>");
					if(parser==NULL)printf("There was an error getting pay date of %s\n",table[i].mapno);
					else{
						string date(parser);
						date.resize(end-parser);
						strcpy(table[i].paydate,date.c_str());
						if((i+1)%50==0)printf("Completed %d\n",i+1);
					}
				}
			}
			else{
                secondLast = parser;
				while((end=strstr(parser+1,"<a href=\"rpcdetail1.cgi?acctnbr="))!=NULL){
                        secondLast = parser;
                        parser=end;
				}
				parser = secondLast;
				parser=strstr(parser,"\"")+1;
				end=strstr(parser,"\"");
				purl=string(parser);
				purl.resize(end-parser);
				purl="acpass.andersoncountysc.org/"+purl;
				printf("\tEntry : %s\n",purl.c_str());

				/*** PAY PAGE ***/
				headerlist=NULL;
				free(chunk.memory);
				chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
				chunk.size = 0;    /* no data at this point */
				curl = curl_easy_init();
				headerlist = curl_slist_append(headerlist, buf);
				if(curl){
					curl_easy_setopt(curl, CURLOPT_URL, purl.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&chunk));
					curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
					res = curl_easy_perform(curl);
					while(res != CURLE_OK){
						fprintf(stderr, "mapno:%s::curl_easy_perform() failed: %s\n",table[i].mapno,curl_easy_strerror(res));
						res = curl_easy_perform(curl);
					}
					curl_easy_reset(curl);
					curl_slist_free_all (headerlist);

					/*** PARSE ***/
					parser=strstr(chunk.memory,"ptaxesno.gif");
					if(parser==NULL)table[i].ptimg=false;
					else table[i].ptimg=true;
					parser=strstr(chunk.memory,"Paid Date");
					parser=strstr(parser,"<strong>")+8;
					end=strstr(parser,"</strong>");
					if(parser==NULL)printf("There was an error getting pay date of %s\n",table[i].mapno);
					else{
						string date(parser);
						date.resize(end-parser);
						strcpy(table[i].paydate,date.c_str());
						if((i+1)%50==0)printf("Completed %d\n",i+1);
					}
				}
			}

		}
		free(chunk.memory);
		if(i+1==table.size())break;
	}
}


void clearRow(row * r){
	r->tmn[0]=r->mv[0]=r->name[0]=r->num[0]=r->district[0]=r->lotdesc[0]=r->mapno[0]=r->db[0]=r->dp[0]
	=r->yd[0]=r->td[0]=r->acre[0]=r->ratio[0]=r->addr[0]=r->prefaddr[0]=r->pv[0]=r->tv[0]=r->lots[0]
	=r->numbldg[0]=r->landass[0]=r->bldgass[0]=r->totass[0]=r->rc[0]=r->paydate[0]=r->img=r->ptimg=0;
	//zlibVersion();
}
