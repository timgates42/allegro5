/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Mac files support.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/aintmac.h"
#include <string.h>

#define TRACE_MAC_FILE 0

/*main volume name*/
char volname[32]="\0\0";
/*main volume number*/
short MainVol=0;
/*main directory ID*/
long MainDir=0;

Str255 mname="\0";

static short parent(short vRefNum, long dirID);
static char *getfilename(const char *fullpath);
static void getpath(char *filename);
static long finddirect(void *pdta);
void getcwd(char *p, int size);


/*
 *
 */
static void getfullpath(short vRefNum,long dirID, char * fullname)
{
   CInfoPBRec myPB;
   Str255 dirName;
   char tmp1[1024];
   char tmp2[1024];
   OSErr myErr;
   dirName[0]=0;
   dirName[1]=0;
   dirName[2]=0;
   dirName[3]=0;
   
   strcpy(fullname,"");
   myPB.dirInfo.ioNamePtr = dirName;
   myPB.dirInfo.ioVRefNum = vRefNum;
   myPB.dirInfo.ioDrParID = dirID;
   myPB.dirInfo.ioFDirIndex = -1;

   myErr = PBGetCatInfoSync(&myPB);

   ptoc(dirName,tmp2);
   if(tmp2[0]){
      strcpy(fullname,"/");
      strcat(fullname,tmp2);
   }
   
   while(myPB.dirInfo.ioDrDirID != fsRtDirID){
      myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;
      myErr = PBGetCatInfoSync(&myPB);

      strcpy(tmp1,fullname);
      ptoc(dirName,tmp2);  
      strcpy(fullname,"/");
      strcat(fullname,tmp2);
      strcat(fullname,tmp1);
   }
   printf("%s\n",fullname);
}



/*
 * this routine get the parent id of dir or file by ours id
 */
static short parent(short vRefNum,long dirID)
{
   FSSpec spec;
   FSMakeFSSpec(vRefNum,dirID,NULL,&spec);
   return spec.parID;
}



#pragma mark Find First
/*
 *code modifyed from allegro/src/libc.c
 */
struct ff_match_data
{
   int type;
   const char *s1;
   const char *s2;
};
typedef struct ff_match_data ff_match_data;
typedef ff_match_data *ffmatchdataPtr;
#define FF_MATCH_TRY 0
#define FF_MATCH_ONE 1
#define FF_MATCH_ANY 2



/* ff_match:
 *Match two strings ('*' matches any number of characters,
 *'?' matches any character).
 */
static int ff_match(const char *s1, const char *s2)
{
   static int size = 0;
   static ffmatchdataPtr data = 0;
   const char *s1end = s1 + strlen(s1);
   int index, c1, c2;
   /* allocate larger working area if necessary */
   if ((data != 0) && (size < strlen(s2))){
      free(data);
      data = 0;
   }
   if (data == 0){
      size = strlen(s2);
      data = (ffmatchdataPtr)malloc(sizeof(ff_match_data)* size * 2 + 1);
      if (data == 0)
         return 0;
   }

   index = 0;
   data[0].s1 = s1;
   data[0].s2 = s2;
   data[0].type = FF_MATCH_TRY;

   while (index >= 0){
      s1 = data[index].s1;
      s2 = data[index].s2;
      c1 = *s1;
      c2 = *s2;

      switch (data[index].type){
         case FF_MATCH_TRY:
            if (c2 == 0){
               /* pattern exhausted */
               if (c1 == 0)
                   return 1;
               else
                   index--;
            }
            else if (c1 == 0){
               /* string exhausted */
               while (*s2 == '*')
                  s2++;
               if (*s2 == 0)
                  return 1;
               else
                  index--;
            }
            else if (c2 == '*'){
               /* try to match the rest of pattern with empty string */
               data[index++].type = FF_MATCH_ANY;
               data[index].s1 = s1end;
               data[index].s2 = s2 + 1;
               data[index].type = FF_MATCH_TRY;
            }
            else if ((c2 == '?') || (c1 == c2)){
               /* try to match the rest */
               data[index++].type = FF_MATCH_ONE;
               data[index].s1 = s1 + 1;
               data[index].s2 = s2 + 1;
               data[index].type = FF_MATCH_TRY;
            }
            else
               index--;
            break;
         case FF_MATCH_ONE:
              /* the rest of string did not match, try earlier */
            index--;
            break;
         case FF_MATCH_ANY:
              /* rest of string did not match, try add more chars to string tail */
            if (--data[index + 1].s1 >= s1) {
               data[index + 1].type = FF_MATCH_TRY;
               index++;
            }
            else
               index--;
            break;
         default:
             /* this is a bird? This is a plane? No it's a bug!!! */
            return 0;
      }
   }
   return 0;
}



/*
 *
 */
static char *getfilename(const char *fullpath)
{
   char *p = (char*)fullpath + strlen(fullpath);
   while ((p > fullpath) && (*(p - 1) != '/')&&(*(p - 1) != '\\'))p--;
   return p;
}



/*
 *
 */
static void getpath(char *filename)
{
   char *temp,*d,*s,*last;
   int div;
   temp=malloc(strlen(filename)+1);
   if(temp!=NULL){
      d=temp;
      div=0;
      last=d;
      for(s=(char *)filename;*s;s++){
         if(*s=='/'||*s=='\\'){
            if(!div){
               *d++='/';
               last=d;
               div=1;
            }
         }
         else{
            *d++=*s;
            div=0;
         }
      }
      *d=0;
      *last=0;
      strcpy(filename,temp);
      free(temp);
   }
}

#define FF_MAXPATHLEN 1024
struct ffblk{
   char dirname[FF_MAXPATHLEN];
   char pattern[FF_MAXPATHLEN];
   int   dattr;
   long dirbase;
   long attrib;
   CInfoPBRec cpb;
};
typedef struct ffblk ffblk;
typedef ffblk *ffblkPtr;



/*
 *
 */
static long finddirect(void *pdta){
   ffblkPtr dta=(ffblkPtr)pdta;   
   char *p,cname[256];
   OSErr err=noErr;       
   int done;
   short buffersize=strlen(dta->dirname);
   char *buffer=malloc(buffersize+1);

   if(!buffer)return 0;
   strcpy(buffer,dta->dirname);

   dta->dirbase=MainDir;
   
   p=buffer;
   
   UppercaseStripDiacritics(buffer,buffersize,smSystemScript);
   if(p[0]=='/'){
      dta->dirbase=fsRtDirID;
      p++;
      p=strtok(p,"\\/");
      if(strcmp(p,volname)==0)
         p=strtok(NULL,"\\/");
   }
   else
      p=strtok(p,"\\/");
   
   while(p!=NULL&&err==noErr&&(*p!=0)){
      if(strcmp(p,".")==0){}
      else if(strcmp(p,"..")==0)
         dta->dirbase=parent(dta->cpb.dirInfo.ioVRefNum,dta->dirbase);
      else if(strcmp(p,"...")==0)
         dta->dirbase=parent(dta->cpb.dirInfo.ioVRefNum,parent(dta->cpb.dirInfo.ioVRefNum,dta->dirbase));
      else{
         for(dta->cpb.dirInfo.ioFDirIndex=1, done=false;
               err == noErr && done != true;
               dta->cpb.dirInfo.ioFDirIndex++){
            dta->cpb.dirInfo.ioDrDirID=dta->dirbase;
            dta->cpb.dirInfo.ioACUser = 0;
            if ((err = PBGetCatInfoSync(&dta->cpb)) == noErr){
               if (dta->cpb.dirInfo.ioFlAttrib&ioDirMask){               
                  ptoc(dta->cpb.dirInfo.ioNamePtr,cname);
                  UppercaseStripDiacritics(cname,strlen(cname),smSystemScript);
                  if(strcmp(p,cname)==0){
                     dta->dirbase=dta->cpb.dirInfo.ioDrDirID;
                     done=true;
                  }
               }
            }
         }
      }
      p=strtok(NULL,"\\/");
   }
   dta->cpb.dirInfo.ioFDirIndex=1;
   free(buffer);
   if(err)dta->dirbase=0;
   return dta->dirbase;
}




/*
 *
 */
void _al_findclose(void *dta)
{
   if(dta)free(dta);
}



/*
 *
 */
int _al_findnext(void *pdta, char *nameret, int *aret)
{
   ffblkPtr dta=(ffblkPtr)pdta;
   char cname[256];
   OSErr err=noErr;
   int curattr;
   int list;
   
#if (TRACE_MAC_FILE)
   fprintf(stdout,"_al_findnext()p=%s\n",dta->pattern);
   fflush(stdout);
#endif
   if(aret)*aret=0;
   for(;err == noErr;dta->cpb.hFileInfo.ioFDirIndex++){
      dta->cpb.hFileInfo.ioDirID = dta->dirbase;
      dta->cpb.hFileInfo.ioACUser = 0;
      if ((err = PBGetCatInfoSync(&dta->cpb)) == noErr){
         ptoc(dta->cpb.hFileInfo.ioNamePtr,cname);
         UppercaseStripDiacritics(cname,strlen(cname),smSystemScript);
         list=true;
         curattr=(dta->cpb.dirInfo.ioFlAttrib & kioFlAttribDirMask)?FA_DIREC:0;
	 curattr|=(dta->cpb.dirInfo.ioFlAttrib &
	          (kioFlAttribLockedMask | kioFlAttribCopyProtMask))
		  ?FA_RDONLY:0;
         if(curattr & ~dta->dattr)list=false;
         if(list){
            if(ff_match(cname, dta->pattern)){
               if(nameret){
                  strcpy(nameret,dta->dirname);
                  strcat(nameret,cname);
               }
               if(aret)*aret=curattr;
               err='OK';
            }
         }
      }
   }
   if(err=='OK'){
      return (*allegro_errno=errno=0);
   }
   return (*allegro_errno=errno=ENOENT);
}



/*
 *
 */
void * _al_findfirst(const char *pattern,int attrib, char *nameret, int *aret)
{
   ffblkPtr dta=(ffblkPtr)malloc(sizeof(ffblk));
   
#if (TRACE_MAC_FILE)
   fprintf(stdout,"_al_findfirst(%s, %d, .,.)\n",pattern,attrib);
   fflush(stdout);
#endif
   if(volname[0]==0)
      _mac_file_sys_init();

   if (dta){ 
      dta->attrib = attrib;
      strcpy(dta->dirname, pattern);
      getpath(dta->dirname);
      strcpy(dta->pattern, getfilename(pattern));   
      if (dta->dirname[0] == 0)strcpy(dta->dirname, "./");
      if (strcmp(dta->pattern, "*.*") == 0)strcpy(dta->pattern, "*");
      UppercaseStripDiacritics(dta->pattern,strlen(dta->pattern),smSystemScript);
      dta->cpb.dirInfo.ioCompletion=NULL;
      dta->cpb.dirInfo.ioVRefNum=MainVol;
      dta->cpb.dirInfo.ioNamePtr=mname;
#if (TRACE_MAC_FILE)
      fprintf(stdout,"directory name %s\n",dta->dirname);
      fflush(stdout);
#endif
      if((finddirect(dta))!=0){
         getfullpath(MainVol,dta->dirbase,dta->dirname);
	 strcat(dta->dirname,"/");
         dta->dattr=attrib;
	 if(attrib & FA_DIREC){
            if(nameret){
               strcpy(nameret,dta->dirname);
               strcat(nameret,"..");
            }
            if(aret)
	       *aret=FA_DIREC;
         }
         else
	    if ((_al_findnext(dta,nameret,aret))!=0){
#if (TRACE_MAC_FILE)
            fprintf(stdout,"_al_findfirst failed no files found\n");
            fflush(stdout);
#endif
            _al_findclose(dta);
            dta=NULL;
         }
      }
      else{
#if (TRACE_MAC_FILE)
         fprintf(stdout,"_al_findfirst failed direct not found\n");
         fflush(stdout);
#endif
         _al_findclose(dta);
         dta=NULL;
      }
   }
#if (TRACE_MAC_FILE)
   else {fprintf(stdout,"_al_findfirst lowmem\n");
   fflush(stdout);}
#endif
   
#if (TRACE_MAC_FILE)
   if(dta==NULL)fprintf(stdout,"_al_findfirst failed\n");
#endif
   if(dta!=NULL)
      *allegro_errno=0;
   return dta;
}
int _al_file_isok(const char *filename){
#if (TRACE_MAC_FILE)
   fprintf(stdout,"_al_file_isok(%s)\n",filename);
   fflush(stdout);
#else
#endif
   return true;
}



/*
 *
 */
int _al_file_exists(const char *filename, int attrib, int *aret){
   void *dta;
   int x=false;
   
   dta=_al_findfirst(filename,attrib,NULL,aret);
   if(dta){
      _al_findclose(dta);
      x=true;
   }
#if (TRACE_MAC_FILE)
   fprintf(stdout,"fileexists(%s)=%d\n",filename,x);
   fflush(stdout);
#endif
   return x;
}



/*
 *
 */
long _al_file_size(const char *filename){
   void *dta;
   long fs;
   
   dta = _al_findfirst(filename,0/*FA_RDONLY | FA_HIDDEN | FA_ARCH*/,NULL,NULL);
   if (dta){
      fs=((ffblkPtr)dta)->cpb.hFileInfo.ioFlLgLen;   
      _al_findclose(dta);
   }
   else {fs=0;}
#if (TRACE_MAC_FILE)
   fprintf(stdout,"filesize(%s)=%d\n",filename,fs);   
   fflush(stdout);
#endif
   return fs;
}



/*
 *
 */
long _al_file_time(const char *filename){
   void *dta;
   long ft;
   
   dta = _al_findfirst(filename,0/*FA_RDONLY | FA_HIDDEN | FA_ARCH*/,NULL,NULL);
   if (dta){
      ft=((ffblkPtr)dta)->cpb.hFileInfo.ioFlMdDat;   
      _al_findclose(dta);
   }
   else {ft=0;}
#if (TRACE_MAC_FILE)
   fprintf(stdout,"filetime(%s)=%d\n",filename,ft);
   fflush(stdout);
#endif
   return ft;
}
int _al_getdrive(void){   
#if (TRACE_MAC_FILE)
   fprintf(stdout,"_al_getdrive()=0\n");
   fflush(stdout);
#endif
   return 0;
}



/*
 *
 */
void _al_getdcwd(int drive, char *buf, int size){
   if(drive==0)
   {
      char * fullname;
      fullname=malloc(1024);
      if(volname[0]==0)
         _mac_file_sys_init();

      getfullpath(MainVol,MainDir,fullname);
#if (TRACE_MAC_FILE)
      fprintf(stdout,"_al_getdcwd(%d,%s,&d)\n",drive,fullname,size);
      fflush(stdout);
#endif
      if(strlen(fullname)<size)
      {
         strcpy(buf,fullname);
      }
      else
      {
         strcpy(buf,"");
      }
   }
   else
   {
#if (TRACE_MAC_FILE)
      fprintf(stdout,"_al_getdcwd(%d,...,&d)\n",drive,size);
      fflush(stdout);
#endif
   }
}



/*
 * an mac replace for open function called from /src/file.c
 */
int _al_open(const char *filename,int mode){
   char *tmp;
   char *path;
   char *s,*d;
   int handle=-1;
   int div;
   int size;
   if(volname[0]==0)
      _mac_file_sys_init();
   size=strlen(filename)+32;
   
   path=malloc(size);
   tmp=malloc(size);
   if(path!=NULL&&tmp!=NULL){
      s=(char *)filename; 
      if(*s=='/')
      {
         s++;
         for(d=tmp;(*s!=0)&&(*s!='/');)*d++=*s++;
	 *d=0;
	 UppercaseStripDiacritics(tmp,strlen(tmp),smSystemScript);
         if(strcmp(tmp,volname)!=0){
	    s=(char *)filename;
	    s++;
	 }
	 strcpy(path,volname);
	 d=path+strlen(volname);
	 *d++=':';
         div=1;
      }
      else{
         div=1;
         d=path;
         *d++=':';
         s=(char *)filename;
         if(*s=='.')s++;
         while(*s=='.'){
	    *d++=':';s++;
	 }
      }
      for(;*s;s++){
         if(*s=='/'||*s=='\\'){
            if(!div){
               *d++=':';
               div=1;
            }
         }
         else{
            *d++=*s;
            div=0;
         }
      }
      *d=0;
#if (TRACE_MAC_FILE)
      fprintf(stdout,"_al_open(%s(%s))\n",filename,path);
      fflush(stdout);
#endif
      handle=open(path,mode);
   }
   free(path);
   free(tmp);
#if (TRACE_MAC_FILE)
   fprintf(stdout,"_al_open(%s ,%d)=%d\n",filename,mode,handle);
   fflush(stdout);
#endif
   return handle;
}



/*
 *
 */
void getcwd(char* p,int size)
{
   _al_getdcwd(0 , p , size);
}



/*
 *
 */
long GetLogicalDrives()
{
   return 1;
}



/*
 *
 */
int _mac_file_sys_init(){
   Str255 name;
   HGetVol(name,&MainVol,&MainDir);
   ptoc(name,volname);
   UppercaseStripDiacritics(volname,strlen(volname),smSystemScript);
   return 0;
}