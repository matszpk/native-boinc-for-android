/*
harmoniousb - an application dedicated to verification of the harmonious tree conjecture
Copyright (C) 2011  Wenjie Fang (aka fwjmath)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contact: fwjmath@gmail.com
*/

#include <stdio.h>
//#include <io.h>
#include <stdlib.h>
#include <unistd.h>
#include "boinc_api.h"
#include "filesys.h"

#define SAaccept 0.5
#define RAND_MAX 0x7fff

//#define COUNTDEPTH 1

FILE* fio;
signed char layout[40], label[40], labelm[40], olayout[40], olabel[40], oolayout[40];
int adj[40][40], cnt[40], labelused[40], sideused[40], isonode[40], isonoderev[40], tail[40], jumping[40], side[800], tmpcts[40];
int node, hl, DFSThreshold, tricount, hardcount, sidenum, nodem1;
unsigned int rnum;
long long count, headcount, dfscount, matchcount, workpoint, totalheadcount;

int nset[40], lset[40], sset[40];
int matrix[40][40], nassign[40], sideset[40][40], nvalidcnt[40], lvalidcnt[40], svalidcnt[40], queue[40][2], fcnt[40];

#ifdef COUNTDEPTH
long long depthcount[40];
#endif

/************************************/
/*                                  */
/* Random Number Generation Utility */
/*                                  */
/************************************/

int myrand(){
	rnum=(rnum*214013L)+2531011L;
	return (int)((rnum>>16)&RAND_MAX);
}

/***************************/

/***************************/
/*                         */
/* Tree Generation Utility */
/*                         */
/***************************/

int isvalid(){
	int i, m, m1, m2;
	m=2;
	while((m<node)&&(layout[m]>1)) m++;
	m1=layout[1];
	i=2;
	while(layout[i-1]<layout[i]) i++;
	m1=layout[i-1];
	m1--;
	m2=1;
	i=m+1;
	while(layout[i-1]<layout[i]) i++;
	m2=layout[i-1];
	if(m2<m1) return m;
	else if(m2==m1){
		if((m<<1)>node+2) return m; 
		else if((m<<1)==node+2){
			i=2;
			while((i<m)&&(layout[i]-1==layout[i+m-2])) i++;
			if(m==i) return 0; 
			if(layout[i+m-2]>=layout[i]-1) return 0; else return m;
		}
		else return 0;
	}
	else return 0;
}

bool nexttree(){
	int i, p, q, v;
	bool flag;
	//generate next tree with skip
	p=node-1;
	while((p>0)&&(layout[p]==1)) p--;
	if(p==0) return false;
	q=p-1;
	v=layout[p]-1;
	while(layout[q]!=v) q--;
	q-=p;
	for(i=p;i<node;i++) layout[i]=layout[i+q];
	v=isvalid();
	if(v!=0){
		v--;
		flag=(layout[v]>2);
		q=v-1;
		p=layout[v]-1;
		while(layout[q]!=p) q--;
		q-=v;
		for(i=v;i<node;i++) layout[i]=layout[i+q];
		if(flag){
			q=2;
			p=2;
			while(p<v){
				if(q<layout[p]) q=layout[p];
				p++;
			}
			p=node-q-1;
			for(i=1;i<=q;i++) layout[p+i]=i;
		}
	}
	return true;
}

void findhead(){
	int i;
	i=2;
	while(layout[i]!=1) i++;
	hl=i;
	return;
}

void addtail(){
	int i, j;
	if((hl<<1)>node+2){
		i=0;
		while(layout[i]<layout[i+1]) i++;
		for(j=1;j<=i;j++) layout[j+hl-1]=j;
		for(j=hl+i;j<node;j++) layout[j]=1;
	}else if((hl<<1)<node+2){
		i=0;
		while(layout[i]<layout[i+1]) i++;
		i--;
		for(j=1;j<=i;j++) layout[j+hl-1]=j;
		for(j=hl+i;j<node;j++) layout[j]=1;
	}else{
		for(j=2;j<hl;j++) layout[hl-2+j]=layout[j]-1;
	}
}

bool nexttreehead(){
	int i, p, q, v;
	bool flag;
	//generate next tree with skip
	p=node-1;
	while((p>0)&&(layout[p]==1)) p--;
	if(p==0) return false;
	q=p-1;
	v=layout[p]-1;
	while(layout[q]!=v) q--;
	q-=p;
	if(p<hl) return false;
	for(i=p;i<node;i++) layout[i]=layout[i+q];
	v=isvalid();
	if(v!=0){
		v--;
		flag=(layout[v]>2);
		q=v-1;
		p=layout[v]-1;
		while(layout[q]!=p) q--;
		q-=v;
		if(v<hl) return false;
		for(i=v;i<node;i++) layout[i]=layout[i+q];
		if(flag){
			q=2;
			p=2;
			while(p<v){
				if(q<layout[p]) q=layout[p];
				p++;
			}
			p=node-q-1;
			if(p+1<hl) return false;
			for(i=1;i<=q;i++) layout[p+i]=i;
		}
	}
	return true;
}

/***********************************/

/***********************************/
/*                                 */
/* Checkpoint, Initialization, etc */
/*                                 */
/***********************************/

void init(){
	if(access("ckpt.txt",0)!=0){
	  // fio=fopen("data.txt","r");
	  char resolved_name[2000];
	  int retval=boinc_resolve_filename("data.txt", resolved_name, sizeof(resolved_name));
	  if (retval) {printf("can't resolve filename data.txt"); boinc_finish(-1);};
	  fio=boinc_fopen(resolved_name, "r");
		node=0;
		fscanf(fio, "%d\n", &node);
		if(node>0){
		fscanf(fio, "%lld\n%u\n%s\n", &headcount, &rnum, layout);
		count=0;
		hardcount=0;
		dfscount=0;
		matchcount=0;
		workpoint=0;
		nodem1=node-1;
		for(int i=0; i<node; i++) layout[i]-='0';
		}else{
			node=-node;
			fscanf(fio, "%lld\n%lld\n%lld\n%lld\n%d\n%u\n%s\n%s\n%s\n%lld\n", &headcount, &count, &dfscount, &matchcount, &hardcount, &rnum, layout, olayout, olabel, &workpoint);
			for(int i=0; i<node; i++){
				layout[i]-='0';
				olayout[i]-='0';
				olabel[i]-='0';
			}
			nodem1=node-1;
			workpoint=0;
		}
		fclose(fio);
	}else{
		fio=boinc_fopen("ckpt.txt","r");
		fscanf(fio, "%d\n%lld\n%lld\n%lld\n%lld\n%d\n%u\n%s\n%s\n%s\n%lld\n", &node, &headcount, &count, &dfscount, &matchcount, &hardcount, &rnum, layout, olayout, olabel, &workpoint);
		for(int i=0; i<node; i++){
			layout[i]-='0';
			olayout[i]-='0';
			olabel[i]-='0';
		}
		fclose(fio);
		nodem1=node-1;
	}
	int a;
	//fio=fopen("data.txt","r");
	char resolved_name[2000];
	int retval=boinc_resolve_filename("data.txt", resolved_name, sizeof(resolved_name));
	if (retval) {printf("can't resolve filename data.txt"); boinc_finish(-1);};
	fio=boinc_fopen(resolved_name, "r");
	fscanf(fio,"%d%lld", &a, &totalheadcount);
	fclose(fio);
	for(int i=0;i<nodem1;i++) label[i+1]=i;
	label[0]=0;
	for(int i=0;i<node;i++) olayout[i]=-1;
	/*
	Parametre & Performance

	20,1400: 1min34s
	20,1600: 1min37s
	20,1700: 1min33s
	20,1800: 1min34s
	20,2000: 1min34s
	20,2300: 1min34s
	20,2600: 1min34s

	21,2300: 5min59s
	21,2500: 5min54s
	21,2700: 5min56s
	*/
	DFSThreshold=(node-20)*500+2000;
#ifdef COUNTDEPTH
	for(int i=0;i<node;i++) depthcount[i]=0;
#endif
	return;
}

void checkpoint(){
  if (boinc_time_to_checkpoint()) {
    fio=boinc_fopen("ckpt.txt", "w");
    for(int i=0; i<node; i++){
      layout[i]+='0';
      olayout[i]+='0';
      olabel[i]+='0';
    }
    olayout[node]=0; olabel[node]=0;
    fprintf(fio, "%d\n%lld\n%lld\n%lld\n", node, headcount, count, dfscount);
    fprintf(fio, "%lld\n%d\n%u\n%s\n%s\n%s\n%lld\n", matchcount, hardcount, rnum, layout, olayout, olabel, workpoint);
    for(int i=0; i<node; i++){
      layout[i]-='0';
      olayout[i]-='0';
      olabel[i]-='0';
    }
    fclose(fio);
    boinc_checkpoint_completed();
    //if(access("stop.txt",0)==0) exit(0);
    //debug
    //printf("Checkpoint at %lld.\n", count);
    //end debug
  }
  //	fio=boinc_fopen("fraction.txt","w");
  //	fprintf(fio, "%.6f", 1.0-(double)headcount/(double)totalheadcount);
  //	fclose(fio);
  double m=1.0-(double)headcount/(double)totalheadcount;
  if(m<(double)workpoint/(double)500000000000.0) m=(double)workpoint/(double)500000000000.0;
#ifdef PROGRESS
  printf("Progress: %3.5f\n", m*100.0f);
  fflush(stdout);
#endif
  boinc_fraction_done(m);
  return;
}

void printlayout(){
	int i;
	for(i=0;i<node;i++) layout[i]+='0';
	fio=boinc_fopen("tree.txt","a");
	fprintf(fio,"%s\n", layout);
	fclose(fio);
	for(i=0;i<node;i++) layout[i]-='0';
	return;
}

void printlayouts(){
	int i;
	//print tree
	for(i=0;i<node;i++) layout[i]+='0';
	printf("%s is so hard! I give up now. Will be returned.\n", layout);
	for(i=0;i<node;i++) layout[i]-='0';
	return;
}

void printlayoutdebug(){
	int i;
	//print tree
	for(i=0;i<node;i++){
		layout[i]+='0';
		label[i]+='0';
	}
	printf("%s\n%s\n", layout, label);
	for(i=0;i<node;i++){
		layout[i]-='0';
		label[i]-='0';
	}
	return;
}

/*************************/

/*************************/
/*                       */
/* Label Finding Utility */
/*                       */
/*************************/

void mapsides(){
	int m, l;
	// By symmetry, root's label can be determined arbitarily, therefore no need to exchange.
	for(m=1; m<node-1; m++){
		for(l=m+1; l<node; l++){
			if(!((cnt[m]==cnt[l])&&(cnt[m]==1)&&(adj[m][0]==adj[l][0]))){
				side[sidenum]=(m<<8)+l;
				sidenum++;
			}
		}
	}
	return;
}

int SearchMain(int a){
	int tabu[20][2];
	int tmp[40];
	int i, j, k, l, m, t1, n1, n2, times, p, ptr;
	if(sidenum==0) mapsides();
	for(i=0;i<node;i++) tmp[i]=0;
	for(i=1;i<node;i++) tmp[(labelm[i]+labelm[adj[i][0]])%nodem1]++;
	for(i=0;i<nodem1;i++) if(tmp[i]==0) tmp[nodem1]++;
	times=60000;
	for(i=0;i<(node/3);i++){tabu[i][0]=0; tabu[i][1]=0;}
	ptr=0;
	while((tmp[nodem1]!=0)&&(times>0)){
		times-=a;
		k=node*node*node; //penalty
		for(j=0;j<(node<<1);j++){
			m=side[myrand()%sidenum];
			l=m&255;
			m>>=8;
			//exchange label and update list
			for(i=0;i<cnt[l];i++){
				int a = labelm[adj[l][i]]+labelm[l];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			for(i=0;i<cnt[m];i++){
				int a = labelm[adj[m][i]]+labelm[m];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			p=labelm[m];
			labelm[m]=labelm[l];
			labelm[l]=p;
			for(i=0;i<cnt[l];i++){
				int a = labelm[adj[l][i]]+labelm[l];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			for(i=0;i<cnt[m];i++){
				int a = labelm[adj[m][i]]+labelm[m];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			//judge if better
			t1=0;
			for(i=0;i<nodem1;i++) if(tmp[i]==0) t1++;
			for(i=0;i<(node/3);i++) if((m==tabu[i][0])&&(l==tabu[i][1])) break;
			if((t1==0)||((t1<k)&&(i==(node/3)))){
				k=t1;
				n1=l;
				n2=m;
			}
			//restore label and status
			for(i=0;i<cnt[l];i++){
				int a = labelm[adj[l][i]]+labelm[l];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			for(i=0;i<cnt[m];i++){
				int a = labelm[adj[m][i]]+labelm[m];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			p=labelm[m];
			labelm[m]=labelm[l];
			labelm[l]=p;
			for(i=0;i<cnt[l];i++){
				int a = labelm[adj[l][i]]+labelm[l];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			for(i=0;i<cnt[m];i++){
				int a = labelm[adj[m][i]]+labelm[m];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			if(k<tmp[nodem1]) break;
		}
#ifdef ANDROID
		if((k<=tmp[nodem1])||((myrand() < ((RAND_MAX+1)>>1))&&(k!=node*node*node))){ //
#else
		if((k<=tmp[nodem1])||((((double) myrand()/(RAND_MAX+1.0)) < SAaccept)&&(k!=node*node*node))){ //
#endif
			//exchange label and update list
			for(i=0;i<cnt[n1];i++){
				int a = labelm[adj[n1][i]]+labelm[n1];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			for(i=0;i<cnt[n2];i++){
				int a = labelm[adj[n2][i]]+labelm[n2];
				tmp[a<nodem1?a:a-nodem1]--;
			}
			p=labelm[n2];
			labelm[n2]=labelm[n1];
			labelm[n1]=p;
			for(i=0;i<cnt[n1];i++){
				int a = labelm[adj[n1][i]]+labelm[n1];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			for(i=0;i<cnt[n2];i++){
				int a = labelm[adj[n2][i]]+labelm[n2];
				tmp[a<nodem1?a:a-nodem1]++;
			}
			tmp[nodem1]=k;
			tabu[ptr][0]=n2;
			tabu[ptr][1]=n1;
			ptr++;
			if(ptr==(node/3)) ptr=0;
		}
	}
	workpoint+=((60000-times)/a)*300;
	return tmp[nodem1];
}

void CalcIsomorphe(){
	int i, j, k, l;
	bool g;
	for(i=0;i<node;i++) isonode[i]=node;
	i=0;
	for(j=0;j<cnt[i]-1;j++){
		g=true;
		for(k=1;k<adj[i][j+1]-adj[i][j];k++){
			l=k+adj[i][j+1];
			if((l>=node)||(layout[adj[i][j]+k]!=layout[l])){
				g=false;
				break;
			}
		}
		if(g) isonode[adj[i][j+1]]=adj[i][j];
	}
	for(i=1;i<node;i++){
		for(j=1;j<cnt[i]-1;j++){
			g=true;
			for(k=1;k<adj[i][j+1]-adj[i][j];k++){
				l=k+adj[i][j+1];
				if((l>=node)||(layout[adj[i][j]+k]!=layout[l])){
					g=false;
					break;
				}
			}
			if(g) isonode[adj[i][j+1]]=adj[i][j];
		}
	}
	return;
}

int DFSLabel(int a){
	int i, p, q, d;
#ifdef COUNTDEPTH
	depthcount[a-1]++;
#endif
	if(a==nodem1){
		p=0;
		while(sideused[p]!=0) p++;
		p-=label[adj[nodem1][0]];
		p+=(p<0?nodem1:0);
		label[nodem1]=p;
		return 1;
	}
	tricount++;
	d=label[adj[a][0]];
	p=(myrand()%(nodem1-a))+a;
	for(i=0;i<nodem1-a;i++){
		if(p>=nodem1) p=a;
		q=label[p];
		int b = q + d;
		b -= (b<nodem1?0:nodem1);
		int* sideb=&sideused[b];
		if((*sideb==0)&&(label[isonode[a]]<=p)){
			label[p]=label[a];
			*sideb=1;
			label[a]=q;
			if(DFSLabel(a+1)==1) return 1;
			if(tricount>DFSThreshold) return 0;
			label[a]=label[p];
			*sideb=0;
			label[p]=q;
		}
		p++;
	}
	return 0;
}

bool MatchFind(){
	int i, j, k, l, m, tmp1, ncount, lcount, scount, okcount=0;
	int qsize=0;
	//test
	//matchcount++;
	//end test
	//collect leaves, vacant labels and vacant sides
	//nset stores FATHER of the leaf
	for(i=0;i<node;i++) fcnt[i]=0;
	ncount=0;
	for(i=0;i<nodem1;i++){
		if(label[i]==-1){
			nset[ncount]=adj[i][0];
			fcnt[nset[ncount]]++;
			ncount++;
		}
	}
	lcount=0;
	for(i=0;i<nodem1;i++){
		if(labelused[i]==0){
			lset[lcount]=i;
			lcount++;
		}
	}
	workpoint+=lcount*40;
	scount=0;
	for(i=0;i<nodem1;i++){
		if(sideused[i]==0){
			sset[scount]=i;
			scount++;
		}
	}
	// start to construct adjacency matrix
	// matrix[node][label]
	for(i=0;i<ncount;i++)
		for(j=0;j<lcount;j++)
			matrix[i][j]=0;
	for(i=0;i<ncount;i++) nvalidcnt[i]=0;
	for(i=0;i<lcount;i++) lvalidcnt[i]=0;
	for(i=0;i<nodem1;i++) svalidcnt[i]=0;
	for(i=0;i<ncount;i++){
		m=label[nset[i]];
		for(j=0;j<lcount;j++){
			k=m+lset[j];
			k-=(k>=nodem1?nodem1:0);
			if(sideused[k]==0){
				matrix[i][j]=1;
				nvalidcnt[i]++;
				lvalidcnt[j]++;
				sideset[k][svalidcnt[k]]=i+j*node;
				svalidcnt[k]++;
			}
		}
		if(nvalidcnt[i]==0) return false;
	}
	for(i=0;i<lcount;i++) if(lvalidcnt[i]==0) return false;
	j=0;
	for(i=0;i<scount;i++) if(svalidcnt[sset[i]]==0) j++;
	if(j>1) return false;
	// start to do constraint propagation
	while(true){
		if(okcount>=ncount) break;
		if(qsize!=0){
			qsize--;
			i=queue[qsize][0];
			j=queue[qsize][1];
			goto startCSP;
		}
		for(i=0;i<ncount;i++){
			if(nvalidcnt[i]<0) continue;
			else if(nvalidcnt[i]<fcnt[nset[i]]) return false;
			else if(nvalidcnt[i]==fcnt[nset[i]]){
				k=i;
				for(l=0;l<lcount;l++){
					if(matrix[i][l]==1){
						while(nvalidcnt[k]<=0) k++;
						queue[qsize][0]=k;
						queue[qsize][1]=l;
						qsize++;
						k++;
					}
				}
				qsize--;
				i=queue[qsize][0];
				j=queue[qsize][1];
				goto startCSP;
			}
		}
		for(j=0;j<lcount;j++){
			if(lvalidcnt[j]==1){
				for(i=0;i<ncount;i++) if(matrix[i][j]==1) break;
				goto startCSP;
			}
		}
		for(k=0;k<scount;k++){
			l=sset[k];
			if(svalidcnt[l]==1){
				i=sideset[l][0];
				j=i/node;
				i%=node;
				goto startCSP;
			}
		}
		tmp1=node<<2;
		for(k=0;k<ncount;k++){
			int tmp2=label[nset[k]];
			for(l=0;l<lcount;l++){
				if(matrix[k][l]==1){
					m=lset[l]+tmp2;
					m-=(m>=nodem1?nodem1:0);
					m=nvalidcnt[k]+lvalidcnt[l]+svalidcnt[m];
					if(m<tmp1){
						tmp1=m;
						i=k;
						j=l;
					}
				}
			}
		}
startCSP:
		nassign[i]=lset[j];
		okcount++;
		fcnt[nset[i]]--;
		nvalidcnt[i]=0;
		lvalidcnt[j]=-1;
		for(k=0;k<ncount;k++){
			if(matrix[k][j]==1){
				matrix[k][j]=0;
				nvalidcnt[k]--;
				if(nvalidcnt[k]==0) return false;
				l=label[nset[k]]+lset[j];
				l-=(l>=nodem1?nodem1:0);
				tmp1=k+j*node;
				for(m=0;m<svalidcnt[l];m++){
					if(sideset[l][m]==tmp1){
						svalidcnt[l]--;
						sideset[l][m]=sideset[l][svalidcnt[l]];
						break;
					}
				}
			}
		}
		for(k=0;k<lcount;k++){
			if(matrix[i][k]==1){
				matrix[i][k]=0;
				lvalidcnt[k]--;
				if(lvalidcnt[k]==0) return false;
				l=label[nset[i]]+lset[k];
				l-=(l>=nodem1?nodem1:0);
				tmp1=i+k*node;
				for(m=0;m<svalidcnt[l];m++){
					if(sideset[l][m]==tmp1){
						svalidcnt[l]--;
						sideset[l][m]=sideset[l][svalidcnt[l]];
						break;
					}
				}
			}
		}
		l=label[nset[i]]+lset[j];
		l-=(l>=nodem1?nodem1:0);
		for(m=0;m<svalidcnt[l];m++){
			j=sideset[l][m];
			k=j/node;
			j%=node;
			if(matrix[j][k]==1){
				matrix[j][k]=0;
				nvalidcnt[j]--;
				if(nvalidcnt[j]==0) return false;
				lvalidcnt[k]--;
				if(lvalidcnt[k]==0) return false;
			}
		}
		svalidcnt[l]=-1;
		tmp1=0;
		for(i=0;i<scount;i++) if(svalidcnt[sset[i]]==0) tmp1++;
		if(tmp1>1) return false;
	}
	l=0;
	for(i=0;i<ncount;i++){
		while(label[l]!=-1) l++;
		label[l]=nassign[i];
	}
	for(i=0;i<nodem1;i++) sideused[i]=0;
	for(i=1;i<nodem1;i++){
		j=label[i]+label[adj[i][0]];
		j-=(j>=nodem1?nodem1:0);
		sideused[j]=1;
	}
	i=0;
	while(sideused[i]!=0) i++;
	i-=label[adj[nodem1][0]];
	i+=(i<0?nodem1:0);
	label[nodem1]=i;
	return true;
}

int PreMatchDFS(int a){
	int i, p;
	if(a==node){
		workpoint+=tricount;
		tricount+=DFSThreshold;
		if(MatchFind()) return 1; else return 0;
	}
	tricount++;
	if(cnt[a]==1) return PreMatchDFS(a+1);
	p=myrand()%node;
	for(i=0;i<nodem1;i++){
		p++;
		if(p>=nodem1) p=0;
		int b = p + label[adj[a][0]];
		b -= (b<nodem1?0:nodem1);
		if((labelused[p]==0)&&(sideused[b]==0)){
			label[a]=p;
			labelused[p]=1;
			sideused[b]=1;
			if(PreMatchDFS(a+1)==1) return 1;
			if(tricount>DFSThreshold) return 0;
			label[a]=-1;
			labelused[p]=0;
			sideused[b]=0;
		}
	}
	return 0;
}

bool CSPFind(int a){
	for(int t=0;t<a;t++){
		// label all internal nodes
		tricount=0;
		for(int i=0;i<node;i++){
			label[i]=-1;
			labelused[i]=0;
			sideused[i]=0;
		}
		label[0]=0;
		labelused[0]=1;
		if(PreMatchDFS(1)==1) return true;
	}
	return false;
}

bool valid(){
	int i, j;
	int k[40];
	for(i=0;i<node;i++) k[i]=0;
	for(i=1;i<node;i++){
		j=(label[i]+label[adj[i][0]])%nodem1;
		if(k[j]!=0) return false; else k[j]=1;
	}
	for(i=0;i<node;i++) k[i]=0;
	for(i=0;i<node;i++) k[label[i]%nodem1]++;
	for(i=0;i<nodem1;i++) if(k[i]==0) return false;
	return true;
}

void calcTreeStruct(){
	int i, j;
	tmpcts[0]=0;
	for(i=0;i<node;i++) cnt[i]=0;
	for(i=1;i<node;i++){
		tmpcts[layout[i]]=i;
		j=tmpcts[layout[i]-1];
		adj[i][cnt[i]]=j;
		cnt[i]++;
		adj[j][cnt[j]]=i;
		cnt[j]++;
	}
	return;
}

bool setDFS(int isroot){
	int i, j, l, m;
	//Construct graph structure
	calcTreeStruct();
	CalcIsomorphe();
	tricount=0;
	for(i=0;i<node;i++){
		label[i]=i;
		sideused[i]=0;
	}
	if(DFSLabel(1)==1){
		if(valid()){
			if(isroot==1){
				for(i=0;i<=node;i++){
					olayout[i]=layout[i];
					olabel[i]=label[i];
				}
			}
			workpoint+=tricount;
			dfscount++;
			return true;
		}
	}
	workpoint+=tricount;
	return false;
}

void findlabel(){
	int i, j, l, m;
	//Construct graph structure
	calcTreeStruct();
	//Judge for trivial labelling
	i=0;
	while(layout[i]<layout[i+1]) i++;
	j=i;
	while((layout[i]!=1)&&(layout[i]>=layout[i+1])) i++;
	if(layout[i]!=1) goto RealFind;
	while(layout[i]<layout[i+1]) i++;
	while((layout[i]!=1)&&(layout[i]>=layout[i+1])&&(i<node)) i++;
	if((layout[i]!=1)&&(i<node)) goto RealFind;
	while((i<node)&&(layout[i]==1)) i++;
	//labeling
	if(i==node){
		//trivial labelling for caterpillars
		for(i=0;i<node;i++) label[i]=-1;
		i=j;
		l=0;
		while(i!=0){
			label[i]=0;
			for(m=1;m<cnt[i];m++){
				l++;
				label[adj[i][m]]=l;
			}
			l++;
			for(m=0;m<node;m++) if(label[m]>=0) label[m]=l-label[m];
			l--;
			i=adj[i][0];
		}
		while(true){
			l++;
			label[i]=0;
			for(m=2;m<cnt[i];m++){
				l++;
				label[adj[i][m]]=l;
			}
			l++;
			for(m=0;m<node;m++) if(label[m]>=0) label[m]=l-label[m];
			l--;
			if(cnt[i]==1) break;
			i=adj[i][1];
		}
		for(m=0;m<node;m++) label[m]--;
		l=label[j];
		if(l<label[adj[j][0]]) l=label[adj[j][0]];
		j=l+node-1;
		for(m=0;m<node;m++) if(label[m]>=l) label[m]=j-label[m];
		return;
	}
	else{
RealFind:;
		i=node-2;
		while((i>0)&&(olayout[i]=layout[i])) i--;
		if(i<=0){
			for(i=0;i<nodem1;i++){
				label[i]=olabel[i];
				sideused[i]=0;
			}
			for(i=1;i<nodem1;i++) sideused[(label[i]+label[adj[i][0]])%nodem1]=1;
			i=0;
			while(sideused[i]!=0) i++;
			i-=label[adj[nodem1][0]];
			i+=(i<0?nodem1:0);
			label[nodem1]=i;
			if(valid()){
				matchcount++;
				return;
			}
		}
		for(DFSThreshold=500;DFSThreshold<=16000;DFSThreshold<<=1){
			for(i=0;i<node;i++) oolayout[i]=layout[i];
			if(setDFS(1)) return;
			if(setDFS(1)) return;
			if(CSPFind(50)){
				if(valid()){
					for(i=0;i<=node;i++){
						olayout[i]=layout[i];
						olabel[i]=label[i];
					}
					matchcount++;
					return;
				}
			}
			if(((hl-1)<<1)>node){
				while(true){
					i=2;
					while((i<node)&&(layout[i]!=1)){
						layout[i-1]=layout[i]-1;
						i++;
					}
					if(i>=node) break;
					layout[i-1]=1;
					for(;i<node;i++) layout[i]++;
					if(setDFS(0)){
						for(i=0;i<node;i++) layout[i]=oolayout[i];
						return;
					}
				}
				for(i=0;i<node;i++) layout[i]=oolayout[i];
				i=2;
				while(oolayout[i]!=1) i++;
				j=2;
				i++;
				while((i<node)&&(oolayout[i]!=1)){
					layout[j]=oolayout[i];
					i++;
					j++;
				}
				layout[j]=1; j++;
				i=2;
				while(oolayout[i]!=1){
					layout[j]=oolayout[i];
					i++;
					j++;
				}
				while(true){
					i=2;
					while((i<node)&&(layout[i]!=1)){
						layout[i-1]=layout[i]-1;
						i++;
					}
					if(i>=node) break;
					layout[i-1]=1;
					for(;i<node;i++) layout[i]++;
					if(setDFS(0)){
						for(i=0;i<node;i++) layout[i]=oolayout[i];
						return;
					}
				}
			}else{
				i=2;
				while(oolayout[i]!=1) i++;
				j=2;
				i++;
				while((i<node)&&(oolayout[i]!=1)){
					layout[j]=oolayout[i];
					i++;
					j++;
				}
				layout[j]=1; j++;
				i=2;
				while(oolayout[i]!=1){
					layout[j]=oolayout[i];
					i++;
					j++;
				}
				while(true){
					i=2;
					while((i<node)&&(layout[i]!=1)){
						layout[i-1]=layout[i]-1;
						i++;
					}
					if(i>=node) break;
					layout[i-1]=1;
					for(;i<node;i++) layout[i]++;
					if(setDFS(0)){
						//printlayouts();
						for(i=0;i<node;i++) layout[i]=oolayout[i];
						return;
					}
				}
				for(i=0;i<node;i++) layout[i]=oolayout[i];
				while(true){
					i=2;
					while((i<node)&&(layout[i]!=1)){
						layout[i-1]=layout[i]-1;
						i++;
					}
					if(i>=node) break;
					layout[i-1]=1;
					for(;i<node;i++) layout[i]++;
					if(setDFS(0)){
						for(i=0;i<node;i++) layout[i]=oolayout[i];
						return;
					}
				}
			}
			for(i=0;i<node;i++) layout[i]=oolayout[i];
		}
		calcTreeStruct();
		for(i=0;i<=node;i++) labelm[i]=label[i];
		if(CSPFind(300)){
			if(valid()){
				for(i=0;i<=node;i++){
					olayout[i]=layout[i];
					olabel[i]=label[i];
				}
				matchcount++;
				return;
			}
		}
		j=0;
		sidenum=0;
		for(i=0;i<=node;i++) label[i]=labelm[i];
		while(SearchMain(30)!=0){
			j++;
			if(j>300) break;
			for(i=0;i<node;i++){
				labelm[0]=myrand()%nodem1;
				for(i=0;i<nodem1;i++) labelm[i+1]=i;
				for(i=0;i<node;i++){
					l=side[myrand()%sidenum];
					m=labelm[l>>8];
					labelm[l>>8]=labelm[l&255];
					labelm[l&255]=m;
				}
			}
		}
		for(i=0;i<node;i++) label[i]=labelm[i];
		if((j>300)||(!valid())){
			printlayout();
			//printlayouts();
			hardcount++;
			return;
		}
		return;
	}
}

/****************/

/****************/
/*              */
/* Control Part */
/*              */
/****************/
bool HeadGen(){
	char tmp[64];
	int c=0;
	long long twp = workpoint;
	for(int i=0;i<=node;i++) tmp[i]=layout[i];
	findhead();
	if(!nexttreehead()){
		for(int i=0;i<=node;i++) layout[i]=tmp[i];
		if(!nexttree()) return false;
		findhead();
		count++;
		findlabel();
	}else{
		for(int i=0;i<=node;i++) layout[i]=tmp[i];
		findhead();
	}
	while(nexttreehead()){
		count++;
		findlabel();
#ifdef VERBOSE
		printf("%d,%lld,%lld\n",c,headcount,workpoint);
#endif
		c++;
		if(c==20000){
			c=0;
			//			if(workpoint-twp>20000000){
				checkpoint();
				twp=workpoint;
				//			}
			if(workpoint>500000000000.0){
				char resolved_name[2000];
				int retval=boinc_resolve_filename("result.txt", resolved_name, sizeof(resolved_name));
				if (retval) {printf("can't resolve filename result.txt"); boinc_finish(-1);};
				fio=boinc_fopen(resolved_name, "a");
				for(int i=0; i<node; i++){
				  layout[i]+='0';
				  olayout[i]+='0';
				  olabel[i]+='0';
				}
				olayout[node]=0; olabel[node]=0;
				fprintf(fio, "%d\n%lld\n%lld\n%lld\n", -node, headcount, count, dfscount);
				fprintf(fio, "%lld\n%d\n%u\n%s\n%s\n%s\n%lld\n", matchcount, hardcount, rnum, layout, olayout, olabel, workpoint);
				for(int i=0; i<node; i++){
				  layout[i]-='0';
				  olayout[i]-='0';
				  olabel[i]-='0';
				}
				char line[1024];
				FILE *file = fopen ("tree.txt" , "r");
				if (file != NULL) {
				  while(fgets(line , sizeof(line) , file) != NULL){
					fprintf(fio, "%s", line);
				  }
				  fclose (file);
				}

				fclose(fio);
				checkpoint();
				remove("ckpt.txt");
				boinc_finish(0);
			}
		}
	}
	addtail();
	return true;
}

void enumhead(){
	long long myworkpoint=workpoint;
	while(HeadGen()){
		headcount--;
		if(headcount==0) break;
		//		if(workpoint-myworkpoint>20000000){
			checkpoint();
			myworkpoint=workpoint;
			//		}
	}
	return;
}

int main(){
  boinc_init();
	init();
	enumhead();
	// fio=fopen("result.txt","a");
	char resolved_name[2000];
	int retval=boinc_resolve_filename("result.txt", resolved_name, sizeof(resolved_name));
	if (retval) {printf("can't resolve filename result.txt"); boinc_finish(-1);};
	fio=boinc_fopen(resolved_name, "a");
	for(int i=0; i<node; i++) layout[i]+='0';
	fprintf(fio,"%lld\n%lld\n%lld\n%lld\n%d\n%s\n%u\n%lld\n\n", count, dfscount, headcount, matchcount, hardcount, layout, rnum, workpoint);
#ifdef COUNTDEPTH
	for(int i=0;i<node;i++) fprintf(fio,"Layer %d: %lld\n",i,depthcount[i]);
#endif
	char line[1024];
	FILE *file = fopen ("tree.txt" , "r");
	if (file != NULL) {
	  while(fgets(line , sizeof(line) , file) != NULL){
	    fprintf(fio, "%s", line);
	  }
	  fclose (file);
	}

	fclose(fio);
	checkpoint();
	remove("ckpt.txt");
	//debug
	//char c;
	//scanf("%c", &c);
	//end debug
	boinc_finish(0);
}
