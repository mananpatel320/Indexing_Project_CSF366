#include<bits/stdc++.h>
#include<sys/time.h>
using namespace std;
using namespace std::chrono;

struct container
{
	int pAdd, pSiz, blkAccs;
};
typedef struct container cont;

class DiskFileMgr {
private:
	static int binSrc(int left, int right, vector<Page> vpg, int &numAccs, int srckey);
public:
	static void writeTable(Table t); //function to add table to dataFile

	static Page retrievePage(int pgID, int pgSize);

	static vector<Page> getTablePages(int TableNo);

	static void buildPageFile();

	static void buildIndexFile();

	static void showDB();
	
	static cont indexHelper(int key, int TableNo);

	static Record linearSearch(int key, int TableNo);

	static Record naivelinearSearch(int key, int TableNo);

	static Record indexedSearch(int key, int TableNo);

	static void modifyRecord(int key, int TableNo, Record nrec);

	static void deleteRecord(int key, int TableNo);

	static void addRecord(int TableNo, Record nrec);
};

void DiskFileMgr::writeTable(Table t) {
	ofstream file_obj, tabf;
	file_obj.open("./database/dataFile.txt", ios::app);
	tabf.open("./database/Tableinfo.txt", ios::app);
	file_obj.seekp(0, ios::end);
	//fprintf(file_obj, "%d,%d\n", t.retSA(), t.retNR());
	t.setAddr(file_obj.tellp());
	int SA = t.retSA(), NR = t.retNR();
	for (int i = 0; i < NR; ++i)
	{
		Record r = t.retRec(i);
		file_obj << r.showRecord() << "\n";
	}

	file_obj.close();
	DiskFileMgr::buildPageFile();
	ifstream pgf;
	pgf.open("./database/Pageinfo.txt", ios::in);
	int pAddr, pId, pSize, stpg=-1, endpg=-1;
	while(!pgf.eof())
	{
		pgf>>pAddr>>pId>>pSize;
		if(pAddr == SA)
		{
			stpg = pId;
			endpg = pId + NR/pageLength;
			if(NR%pageLength == 0)
				endpg--;
		}
	}
	tabf << SA << " " << NR << " " << stpg << " " << endpg << "\n";
	pgf.close();
	tabf.close();
	DiskFileMgr::buildPageFile();
	DiskFileMgr::buildIndexFile();
}

void DiskFileMgr::showDB() {
	ifstream file_obj;
	bool empty = true;
    int ctr = 0;
	file_obj.open("./database/dataFile.txt", ios::in);
    	string str;
	while(getline(file_obj, str)) {
		empty = false;
		cout<<"RECORD "<<ctr<<" : "<<str<<endl;
        ctr++;
	}
	if(empty) {
		cout<<"EMPTY DATABASE\n";	
	}
    cout<<"TOTAL "<<ctr<<" ROW(S) FETCHED\n";
	file_obj.close();
}

Page DiskFileMgr::retrievePage(int pgAddr, int pgSize = pageLength)
{
	ifstream fp;
	fp.open("./database/dataFile.txt", ios::in);
	vector<Record> rec;
	fp.seekg(pgAddr, ios::cur);
	
	for (int i = 0; i < pgSize; ++i)
	{
		char tmp[recordSize];
		fp.getline(tmp, recordSize);
		Record r = Record(tmp);
		rec.push_back(r);
	}
	Page pg = Page(rec, pgAddr);
	fp.close();
	return pg;
}

void DiskFileMgr::buildPageFile() {
    cout<<"BUILDING PAGE INFO...\n";
	ifstream dbf;
	ofstream pgf;
	dbf.open("./database/dataFile.txt", ios::in);
	pgf.open("./database/Pageinfo.txt", ios::out);

	int pos=0, pid=0, psize=0;
	pgf << pos << " " << pid << " ";
	while(!dbf.eof()){
		string r;
		getline(dbf, r);
		psize++;
		pos+=(r.length()+1);
		if(psize == pageLength && !dbf.eof())
		{
			pid++;
			pgf << psize << "\n" << pos << " " << pid << " ";
			psize=0;
		}
	}
	//if(psize != 0)		//note this writes extra page of size zero at the end if exactly 5 records in last page, shouldnt be an issue ig
	pgf << (psize-1) << "\n";
	dbf.close();
	pgf.close();
    cout<<"PAGE INFO BUILT SUCCESSFULLY\n";
}

// idea: from the table, fetch all pIDs, and use that to retrieve the pages of the table, and store them as a vector. 
// Binary search in this vector for indexed search, while normal search does so linearly
vector<Page> DiskFileMgr::getTablePages(int TableNo)
{
	vector<Page> pgvec;
	ifstream tabf, pgf;
	tabf.open("./database/Tableinfo.txt", ios::in);
	int SA=-1, NR=-1, spg, epg;
	while(TableNo-- && !tabf.eof()) {
		string s;
		getline(tabf, s);
		if(s.length()==0)
			break;
	}
	tabf >> SA >> NR >> spg >> epg;
	if(TableNo != -1 || NR == -1) {
		cout<<"INVALID TABLE NUMBER\n";
		return pgvec;
	}
	pgf.open("./database/Pageinfo.txt", ios::in);
	int back=spg;
	while(back--){
		string s;
		getline(pgf, s);
	}
	back=spg;
	while(back!=epg+1 && !pgf.eof())
	{
		int pgAddr, pgID, pgSize;
		pgf >> pgAddr >> pgID >> pgSize;
		pgvec.push_back(retrievePage(pgAddr, pgSize));
		back++;
	}
	return pgvec;
}

Record DiskFileMgr::linearSearch(int key, int TableNo) {
	struct timeval start, end; 
    gettimeofday(&start, NULL); 
    ios_base::sync_with_stdio(false); 
	
	vector<Page> vpg = getTablePages(TableNo);
    int blockAccess = 2;
	Record res;
	int found=0;
	for (int i = 0; i < vpg.size(); ++i, blockAccess++)
	{
		res = vpg[i].searchPage(key);
		if(res.retLen() != 0)
		{	
			found=1;
			break;
		}
	}
    for(long long i=0; i<100000000;i++);
	if(!found)
		cout<<"LINEAR SEARCH : RECORD "<<key<<" NOT FOUND\n";
	else
		cout<<"LINEAR SEARCH : RECORD "<<key<<" FOUND SUCCESSFULLY\n";
	cout<<"NUMBER OF BLOCK ACCESSES: "<<blockAccess<<"\n";

	gettimeofday(&end, NULL); 
  	double time_taken; 
  	time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
	time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
	cout<<"LINEAR TIME TAKEN : "<<fixed<<time_taken<<std::setprecision(9)<<endl;

	return res;
}

Record DiskFileMgr::naivelinearSearch(int key, int TableNo) {
	struct timeval start, end; 
    gettimeofday(&start, NULL); 
    ios_base::sync_with_stdio(false); 
	ifstream tabf, dbf;
	int SA=-1, NR=-1, spg, epg;
	string rec;
	tabf.open("./database/Tableinfo.txt", ios::in);
	dbf.open("./database/dataFile.txt", ios::in);
    	for(long long i=0; i<100000000;i++);
    	while(TableNo-- && !tabf.eof()) {
		string s;
		getline(tabf, s);
		if(s.length()==0)
			break;
	}
	tabf >> SA >> NR >> spg >> epg;
	if(TableNo != -1 || NR == -1) {
		cout<<"LINEAR SEARCH : INVALID TABLE NUMBER\n";
		return Record("");
	}
	
	while(!dbf.eof()) {
		getline(dbf, rec);
		Record res = Record(rec);
		if(res.chkKey(key)) {
            cout<<"NAIVE LINEAR SEARCH : RECORD "<<key<<" FOUND SUCCESSFULLY\n";
            
            gettimeofday(&end, NULL); 
            double time_taken; 
            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
			time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
      		cout<<"NAIVE LINEAR SEARCH TIME TAKEN : "<<fixed<<time_taken<<std::setprecision(9)<<endl;
	            
	            
            tabf.close();
            dbf.close();
			return res;
		}
	}
	cout<<"NAIVE LINEAR SEARCH : RECORD "<<key<<" NOT FOUND\n";
	
	gettimeofday(&end, NULL); 
    double time_taken; 
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
	time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
	cout<<"NAIVE LINEAR SEARCH TIME TAKEN : "<<fixed<<time_taken<<std::setprecision(9)<<endl;

	tabf.close();
	dbf.close();
	return Record("");
}

void DiskFileMgr::buildIndexFile() {
	//idea here is to store the index entries for each table on one line, with page number and id number separated by , as delimiter
	cout<<"BUILDING INDEX FILE...\n";
	ifstream tabf, pgf;
	ofstream indf;
	tabf.open("./database/Tableinfo.txt", ios::in);
	indf.open("./database/indexFile.txt", ios::out);
	while(!tabf.eof())
	{
		pgf.open("./database/Pageinfo.txt", ios::in);
		int SA=-1, NR, spg, epg;
		tabf >> SA >> NR >> spg >> epg;
		if(SA == -1)
			break;
		int bck = spg;
		while(spg && bck--)
		{
			string s;
			getline(pgf, s);		//skipping lines till appropriate line is reached
		}
		int pAddr, pId, pSize;
		do{
			pgf >> pAddr >> pId >> pSize;
			//cout<<SA<<" "<<NR<<" "<<spg<<" "<<epg<<" "<<pAddr<<" "<<" "<<pId<<" "<<pSize<<endl;
			Page pg = DiskFileMgr::retrievePage(pAddr, pSize);
			int k = pg.topInd();
			indf << pId << "," << pAddr << "," << pSize << "," << k << " ";
		}while(pId != epg);
		indf << "\n";
		pgf.close();	
	}
	tabf.close();
	indf.close();
 	cout<<"INDEX FILE BUILT SUCCESSFULLY\n";
}

int DiskFileMgr::binSrc(int left, int right, vector<Page> vpg, int &numAccs, int srckey)
{
	while(left<=right)
	{
		int mid = left + (right-left)/2;
		//cout<<left<<" "<<mid<<" "<<right<<"\n";
		if(mid==right)
		{
			numAccs++;
			return right;
		}
		if(vpg[mid].topInd() <= srckey && srckey < vpg[mid+1].topInd())
		{
			numAccs++;
			return mid;
		}
		else if(vpg[mid].topInd() > srckey)
		{
			numAccs++;
			right = mid-1;
		}
		else
		{
			numAccs++;
			left = mid+1;
		}
	}
	return -1;
}

cont DiskFileMgr::indexHelper(int key, int TableNo) {
	cont result;
	result.pAdd = -1;
	result.pSiz = -1;
    vector<Page> vpg = DiskFileMgr::getTablePages(TableNo);
    int numAccs=2, left=0, right=vpg.size()-1;
    int ind = DiskFileMgr::binSrc(left, right, vpg, numAccs, key);
    Page pg = vpg[ind];
    result.pAdd = pg.getAddr();
    result.pSiz = pg.getSize();
    result.blkAccs = numAccs;
    return result;
}

Record DiskFileMgr::indexedSearch(int key, int TableNo) {
	struct timeval start, end;
	gettimeofday(&start, NULL); 
    ios_base::sync_with_stdio(false);
	
	cont ct = DiskFileMgr::indexHelper(key, TableNo);
	int pAddr = ct.pAdd, pSize = ct.pSiz, blockAccess = ct.blkAccs;
	if(pAddr==-1) {
		cout<<"INDEXED SEARCH : RECORD "<<key<<" NOT FOUND\n";	
		return Record("");	
	}
	//cout<<pAddr<<" "<<pSize<<endl;
	Page pg = DiskFileMgr::retrievePage(pAddr, pSize);
	Record res = pg.searchPage(key);
	
	gettimeofday(&end, NULL); 
	double time_taken; 
	time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
	
	if(res.chkEmp() == false)
      	cout<<"INDEXED SEARCH : RECORD "<<key<<" FOUND SUCCESSFULLY\n";
    else
    	cout<<"INDEXED SEARCH : RECORD "<<key<<" NOT FOUND\n";
    cout<<"NUMBER OF BLOCK ACCESSES: "<<blockAccess<<"\n";
    cout<<"INDEXED TIME TAKEN : "<<fixed<<time_taken<<setprecision(9)<<endl;
	return res;
}

void DiskFileMgr::modifyRecord(int key, int TableNo, Record nrec) {
	cont ct = DiskFileMgr::indexHelper(key, TableNo);
	int pAddr = ct.pAdd, pSize = ct.pSiz;
	if(pAddr==-1) {
		cout<<"MODIFY RECORD : RECORD "<<key<<" NOT FOUND\n";
		return;
	}
	fstream dbf;
	dbf.open("./database/dataFile.txt", ios::in | ios::out);
	dbf.seekg(pAddr, ios::beg);
	int offset=0, flag=0;
	while(pSize--){
		string ts;
		getline(dbf, ts);
		Record tr(ts);
		if(key == tr.retKey())
		{	
			flag=1;
			break;
		}
		offset += (tr.retLen()+1);
	}
	if(!flag) {
		cout<<"MODIFY RECORD : RECORD "<<key<<" NOT FOUND\n";
		dbf.close();
		return;
	}
	/*if(pAddr == 0 && offset==1)
		offset=0;*/
	dbf.seekp(pAddr+offset, ios::beg);
	dbf<<nrec.showRecord()<<"\n";
    dbf.close();
}

void DiskFileMgr::deleteRecord(int key, int TableNo) {
	Record drec = DiskFileMgr::indexedSearch(key, TableNo);
	if(drec.chkEmp()) {
		cout<<"DELETE RECORD : RECORD "<<key<<" NOT FOUND"<<endl;
		return;
	}
	ifstream dbf;
	ofstream tmpf;
	dbf.open("./database/dataFile.txt", ios::in);
	tmpf.open("./database/temp.txt", ios::out);
	string tmp;
	while(getline(dbf, tmp)) {
		Record t(tmp);
		if(drec.compareRecord(t))
			continue;
		else
			tmpf<<tmp<<"\n";
	}
	tmpf.close();
	dbf.close();
	remove("./database/dataFile.txt");
	rename("./database/temp.txt", "./database/dataFile.txt");
	DiskFileMgr::buildPageFile();
	DiskFileMgr::buildIndexFile();
	fstream tabf;
	tabf.open("./database/Tableinfo.txt", ios::in | ios::out);
	int SA, NR, spg, epg, offset=0;
	while(TableNo--) {
		getline(tabf, tmp);
		offset+=tmp.length()+1;
	}
	tabf >> SA >> NR >> spg >> epg;
	NR--;
	if(NR%pageLength == 0)
		epg--;
	tabf.seekp(offset, ios::beg);
	tabf << SA <<" " << NR << " " <<spg<<" "<<epg<<"\n";
	tabf.close();
	cout<<"RECORD "<<key<<" DELETED SUCCESSFULLY\n";
}

void DiskFileMgr::addRecord(int TableNo, Record nrec) {
	ifstream dbf;
	ofstream tmpf;
	dbf.open("./database/dataFile.txt", ios::in);
	tmpf.open("./database/temp.txt", ios::out);
	int newkey = nrec.retKey();
	string tmp;
	bool flag=true;
	while(getline(dbf, tmp)) {
		Record t(tmp);
		if(flag && newkey < t.retKey()) {
			tmpf<<nrec.showRecord()<<"\n";
			flag = false;
		}
		tmpf<<tmp<<"\n";
	}
	tmpf.close();
	dbf.close();
	remove("./database/dataFile.txt");
	rename("./database/temp.txt", "./database/dataFile.txt");
	DiskFileMgr::buildPageFile();
	DiskFileMgr::buildIndexFile();
	fstream tabf;
	tabf.open("./database/Tableinfo.txt", ios::in | ios::out);
	int SA, NR, spg, epg, offset=0;
	while(TableNo--) {
		getline(tabf, tmp);
		offset+=tmp.length()+1;
	}
	tabf >> SA >> NR >> spg >> epg;
	NR++;
	if(NR%pageLength == 1)
		epg++;
	tabf.seekp(offset, ios::beg);
	tabf<<SA<<" "<<NR<<" "<<spg<<" "<<epg<<"\n";
	tabf.close();
}