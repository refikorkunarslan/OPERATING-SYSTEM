#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <ctime>
#include <mutex>
#include <cmath>
#include <fstream>
using namespace std;
#define thread_max 4
bool found = false;

struct PageTableEntry
{

    PageTableEntry(){}
    PageTableEntry(PageTableEntry& pte);
    int page_index = -1;
    int pmpage_index = -1;
    int modified = 0;  // NRU
    int referenced = 0; // Second-Chance & NRU & LRU & WSClock
};

struct PageTable
{

    PageTable(){}
    ~PageTable();
    void initTable(int max);
    PageTableEntry* table = NULL;
    vector<int> queue; // FIFO, Second-Chance, LRU, WSClock
    int wsCP = 0; // WSClock ( Last Used is pointed! )
    int tableCounter = 0;
    int tableSize = 0;
};
struct PhysicalMemory
{

    PhysicalMemory(){}
    ~PhysicalMemory();
    PhysicalMemory(int frameSize, int numPhysical);
    int** arr = NULL;
    int* valids = NULL;
    int frameSize = -1;
    int numPhysical;
    int physicalMemorySize;
    int physicalPageCounter = 0;
};
struct PageReplacement
{
    PhysicalMemory* pm = NULL;
    ofstream diskFile;
    int page_indexPM = 0;
};

struct VirtualMemory
{
    VirtualMemory(){}
    ~VirtualMemory();
    VirtualMemory(int numVirtual, int numPhysical, int frameSize, int pageTablePrintInt, string allocPolicy, string pageReplacement, string diskFileName);
    PhysicalMemory* pm = NULL;
    PageTable pt;
    PageReplacement pr;
    int** arr = NULL;
    int frameSize = -1;
    int pageTablePrintInt;
    int pageTablePrintIntCounter = 0;
    int numberOfreads = 0;
    int numberOfwrites = 0;
    int numberOfPageMisses = 0;
    int numberOfPageReplacements = 0;
    int numberOfDiskPageWrites = 0;
    int numberOfDiskPageReads = 0;
    string pageReplacement;
    string diskFileName;
    int numVirtual;
    int virtualMemorySize;
};

void set(unsigned int index, int value, char * tName);
int get(unsigned int index, char * tName);
int getValidpage_index();


struct Sorter
{

    Sorter(){}
    Sorter(VirtualMemory* virtualArray);
    VirtualMemory* virtualArray;
    int halfSize;
    string bubbleString = "bubble";
    string quickString = "quick";
    string fillString = "fill";
    string linearString="linear";
    string binaryString="binary";
    
};
struct Search
{
     Search(){}
    Search(VirtualMemory* virtualArray);
    VirtualMemory* virtualArray;
    int halfSize;

};
void pageReplace(string pageReplacement, PageTable& pt, int page_index);
void printPageTable();
void  writeDisk(int removedpage_index);

VirtualMemory* virtualArray = NULL;
Sorter *sort= NULL;
Search *sh=NULL;
int a[100000000];
int part = 0;
int f = 0;
int key = 202;
  
int current_thread = 0;
bool control(unsigned int index, char* tName)
{
    if (index < 0 || index > virtualArray->virtualMemorySize)
    {
        cerr << "Error: Unacceptable index!" << endl;
        return false;
    }
    string threadName(tName);

    if (threadName.compare("bubble") != 0 && threadName.compare("quick") != 0 && threadName.compare("fill") != 0  && threadName.compare("linear") != 0 && threadName.compare("binary") != 0 )
    {
        cerr << "Error: Unacceptable thread name! " << threadName << endl;
        return false; 
    }


    int Page = index / (virtualArray->frameSize);

    if (threadName.compare("bubble") == 0 && Page < virtualArray->numVirtual/2)
        return true;
    else if (threadName.compare("quick") == 0 && (Page >= virtualArray->numVirtual/2) && (Page < virtualArray->numVirtual))
        return true;
    

   cerr << "Error: Unacceptable index" << threadName << endl;
    return false;
}

int PageTableGet(int virtualArraypage_index, int Inpage_index)
{
    int i = 0, currentPageInPT = 0;
    bool isHit = false;
    for (i=0; i < virtualArray->pt.tableCounter; ++i)
    {
        if (virtualArray->pt.table[i].page_index == virtualArraypage_index)
        {
            currentPageInPT = i;
            isHit = true;
            break;
        }
    }

    if (isHit)
    {
        ++virtualArray->numberOfreads;
        ++virtualArray->pageTablePrintIntCounter;

        if (virtualArray->pageReplacement.compare("LRU") == 0 || virtualArray->pageReplacement.compare("SC") == 0 || virtualArray->pageReplacement.compare("FIFO") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
        {
            int queueIndex = 0;
            for (; queueIndex < virtualArray->pt.queue.size(); ++queueIndex)
                if (virtualArray->pt.queue.at(queueIndex) == virtualArraypage_index)
                    break;

            if (virtualArray->pageReplacement.compare("LRU") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
            {
                auto it = virtualArray->pt.queue.begin();
                for (int k = queueIndex; k > 0; ++it, --k);
                virtualArray->pt.queue.erase(it);
                virtualArray->pt.queue.push_back(virtualArraypage_index);
                virtualArray->pt.wsCP=virtualArraypage_index;
            }
        }
        if (virtualArray->pageReplacement.compare("SC") == 0 || virtualArray->pageReplacement.compare("NRU") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
            virtualArray->pt.table[currentPageInPT].referenced=1;

        virtualArray->pt.table[currentPageInPT].modified=0;

        printPageTable();

        return virtualArray->pm->arr[virtualArray->pt.table[currentPageInPT].pmpage_index][Inpage_index];
    }
    else
    {
        ++virtualArray->numberOfPageMisses;
        virtualArray->numberOfwrites += virtualArray->frameSize;
        virtualArray->pageTablePrintIntCounter += virtualArray->frameSize;

        vector<int> virtualPageValues;
        for (i = 0; i < virtualArray->frameSize; ++i, ++virtualArray->numberOfreads)
            virtualPageValues.push_back(virtualArray->arr[virtualArraypage_index][i]);

        if (virtualArray->pt.tableCounter < virtualArray->pt.tableSize)
        {
            int validIndexPM = getValidpage_index();
            int pageTableIndex = virtualArray->pt.tableCounter;

            virtualArray->pt.table[pageTableIndex].modified=1;
            virtualArray->pt.table[pageTableIndex].referenced=0;
            virtualArray->pt.table[pageTableIndex].pmpage_index=validIndexPM;
            virtualArray->pt.table[pageTableIndex].page_index=virtualArraypage_index;
            virtualArray->pt.queue.push_back(virtualArraypage_index);
            virtualArray->pt.wsCP=virtualArraypage_index;
            virtualArray->pt.tableCounter=virtualArray->pt.tableCounter + 1;

            for (i = 0; i < virtualPageValues.size(); ++i)
                virtualArray->pm->arr[validIndexPM][i] = virtualPageValues.at(i);

            virtualArray->pm->valids[validIndexPM]= 0;

            return virtualArray->pm->arr[virtualArray->pt.table[pageTableIndex].pmpage_index][Inpage_index];
        }
        ++virtualArray->numberOfPageReplacements;
        pageReplace(virtualArray->pageReplacement, virtualArray->pt, virtualArraypage_index);
        int validIndexPM = virtualArray->pr.page_indexPM;
        for (i = 0; i < virtualPageValues.size(); ++i)
            virtualArray->pm->arr[validIndexPM][i] = virtualPageValues.at(i);

        virtualArray->numberOfDiskPageWrites += virtualArray->frameSize;
        virtualArray->numberOfDiskPageReads += virtualArray->frameSize;

        printPageTable();

        return virtualArray->pm->arr[validIndexPM][Inpage_index];
    }
}

void PageTableSet(int virtualArraypage_index, int valueIndex, int value)
{
    int i = 0, currentPageInPT = 0;
    bool isHit = false;
    for (; i < virtualArray->pt.tableCounter; ++i)
    {
        if (virtualArray->pt.table[i].page_index == virtualArraypage_index)
        {
            currentPageInPT = i;
            isHit = true;
            break;
        }
    }

    if (isHit)
    {
        ++virtualArray->numberOfreads;
        ++virtualArray->numberOfwrites;
        ++virtualArray->pageTablePrintIntCounter;

        if (virtualArray->pageReplacement.compare("LRU") == 0 || virtualArray->pageReplacement.compare("SC") == 0
            || virtualArray->pageReplacement.compare("FIFO") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
        {
            int queueIndex = 0;
            for (; queueIndex < virtualArray->pt.queue.size(); ++queueIndex)
                if (virtualArray->pt.queue.at(queueIndex) == virtualArraypage_index)
                    break;

            if (virtualArray->pageReplacement.compare("LRU") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
            {
                auto it = virtualArray->pt.queue.begin();
                for (int k = queueIndex; k > 0; ++it, --k);
                virtualArray->pt.queue.erase(it);
                virtualArray->pt.queue.push_back(virtualArraypage_index);
                virtualArray->pt.wsCP=virtualArraypage_index;
            }
        }
        if (virtualArray->pageReplacement.compare("SC") == 0 || virtualArray->pageReplacement.compare("NRU") == 0 || virtualArray->pageReplacement.compare("WSCLOCK") == 0)
            virtualArray->pt.table[currentPageInPT].referenced=1;
        virtualArray->pt.table[currentPageInPT].modified=0;

        virtualArray->arr[virtualArraypage_index][valueIndex] = value;
        virtualArray->pm->arr[virtualArray->pt.table[currentPageInPT].pmpage_index][valueIndex] = value;
    }
    else
    {
        ++virtualArray->numberOfPageMisses;
        virtualArray->numberOfwrites += virtualArray->frameSize;
        virtualArray->pageTablePrintIntCounter += virtualArray->frameSize;

        virtualArray->arr[virtualArraypage_index][valueIndex] = value;

        vector<int> virtualPageValues;
        for (i = 0; i < virtualArray->frameSize; ++i, ++virtualArray->numberOfreads)
            virtualPageValues.push_back(virtualArray->arr[virtualArraypage_index][i]);

        if (virtualArray->pt.tableCounter < virtualArray->pt.tableSize)
        {
            int validIndexPM = getValidpage_index();
            int pageTableIndex = virtualArray->pt.tableCounter;

            virtualArray->pt.table[pageTableIndex].modified=1;
            virtualArray->pt.table[pageTableIndex].referenced=0;
            virtualArray->pt.table[pageTableIndex].pmpage_index=validIndexPM;
            virtualArray->pt.table[pageTableIndex].page_index=virtualArraypage_index;
            virtualArray->pt.queue.push_back(virtualArraypage_index);
            virtualArray->pt.wsCP=virtualArraypage_index;
            virtualArray->pt.tableCounter=virtualArray->pt.tableCounter + 1;

            for (i = 0; i < virtualPageValues.size(); ++i)
                virtualArray->pm->arr[validIndexPM][i] = virtualPageValues.at(i);

            virtualArray->pm->valids[validIndexPM]= 0;

            virtualArray->pm->arr[virtualArray->pt.table[pageTableIndex].pmpage_index][valueIndex] = value;
        }
        else
        {
            ++virtualArray->numberOfPageReplacements;
            pageReplace(virtualArray->pageReplacement, virtualArray->pt, virtualArraypage_index);
            int validIndexPM = virtualArray->pr.page_indexPM;
            for (i = 0; i < virtualPageValues.size(); ++i)
                virtualArray->pm->arr[validIndexPM][i] = virtualPageValues.at(i);

            virtualArray->pm->arr[validIndexPM][valueIndex] = value;
            virtualArray->numberOfDiskPageWrites += virtualArray->frameSize;
            virtualArray->numberOfDiskPageReads += virtualArray->frameSize;
        }
    }
    printPageTable();
}


VirtualMemory::VirtualMemory(int numVirtual, int numPhysical, int frameSize, int pageTablePrintInt, string allocPolicy, string pageReplacement, string diskFileName)
{
    this->numVirtual = pow(2, numVirtual);
    this->frameSize = pow(2, frameSize);
    this->pageTablePrintInt = pageTablePrintInt;
    this->pageReplacement = pageReplacement;
    this->diskFileName = diskFileName;
    this->virtualMemorySize = this->frameSize * this->numVirtual;
    this->pm = new PhysicalMemory(this->frameSize, numPhysical);
    int pageTableSize = this->pm->numPhysical / (this->frameSize / 1024);
    this->pt.initTable(pageTableSize);
    this->arr = new int*[this->numVirtual];
    this->pr.diskFile.open(diskFileName.c_str());
    this->pr.pm=pm;
    for (int i = 0; i < this->numVirtual; ++i)
        this->arr[i] = new int[this->frameSize];
}

void printPageTable()
{
    if (virtualArray->pageTablePrintIntCounter >= virtualArray->pageTablePrintInt)
    {
       
        int i = 0;
        for (; i < virtualArray->pt.tableCounter; ++i)
        {
            cout << "\t\t" << "Page Table Entry " << "Virtual  Memory Page: " << virtualArray->pt.table[i].page_index<< endl;
        }
        if (virtualArray->pageReplacement.compare("SC") || virtualArray->pageReplacement.compare("FIFO")
         || virtualArray->pageReplacement.compare("LRU") || virtualArray->pageReplacement.compare("WSCLOCK"))
        {
            cout << "\tlast used page table  " << virtualArray->pageReplacement << " :" << endl;
            cout << "\t\t";
            int j = 0;
            for (; j < virtualArray->pt.queue.size(); ++j)
                cout << virtualArray->pt.queue.at(j) << ", ";
            cout << endl;
        }

        virtualArray->pageTablePrintIntCounter = 0;
    }
}

void printStatistics()
{
    cout << "Number of Reads\t\t: " << virtualArray->numberOfreads << endl;
    cout << "Number of Writes\t\t: " <<virtualArray->numberOfwrites << endl;
    cout << "Number of Page Misses\t\t: " << virtualArray->numberOfPageMisses << endl;
    cout << "Number of Page Replacements\t\t: " << virtualArray->numberOfPageReplacements << endl;
    cout << "Number of Disk Page Writes\t\t: " << virtualArray->numberOfDiskPageWrites << endl;
    cout << "Number of Disk Page Reads\t\t: " << virtualArray->numberOfDiskPageReads << endl;
}

int sortControl()
{
    int i = 0, j = 0;
       

    for (; i < (virtualArray->numVirtual/2) ; ++i)
        for (j = 0; j < virtualArray->frameSize-1; ++j)
            if (virtualArray->arr[i][j] > virtualArray->arr[i][j+1])
            {
               return 0;
            }

    for (i = virtualArray->numVirtual/2; i < (virtualArray->numVirtual); ++i)
        for (j = 0; j < virtualArray->frameSize-1; ++j)
            if (virtualArray->arr[i][j] > virtualArray->arr[i][j+1])
            {
                return 0;
            }
     


  return 1; 

}


void * fill(void * arg)
{
    VirtualMemory * virt = (VirtualMemory *) arg;
   virt->pt.initTable( virt->virtualMemorySize);

    srand(1000);
    for (int i = 0; i <  virt->virtualMemorySize; i++) {
        int randNum = rand() % 100;
       set(i, randNum, (char*)sort->fillString.c_str());
    }

    pthread_exit((void *) NULL);
}


VirtualMemory::~VirtualMemory()
{
    if (this->pm != NULL)
    {
        for (int i = 0; i < this->numVirtual; ++i)
            delete [] this->arr[i];
        delete [] this->arr;
        delete this->pm;
    }
}
void pageReplace(string pageReplacement, PageTable& pt, int page_index)
{
    if (pageReplacement.compare("NRU") == 0)
    {
            int i = 0;

            for (; i < pt.tableSize; ++i)
            {
                if (pt.table[i].modified == 0 && pt.table[i].referenced == 0)
                    break;
            }
            if (i == pt.tableSize)
                i = 0;

            writeDisk(i);

            pt.table[i].page_index=page_index;

            virtualArray->pm->valids[pt.table[i].pmpage_index]= 1;
            int validPMIndex = getValidpage_index();
            virtualArray->pr.page_indexPM = validPMIndex;

            pt.table[i].pmpage_index=validPMIndex;
    }
    else if (pageReplacement.compare("FIFO") == 0)
    {
            int replaceIndex = pt.queue.at(0), pageTableIndex = 0;
            for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                if (replaceIndex == pt.table[pageTableIndex].page_index)
                    break;

            PageTableEntry ptReplace = pt.table[pageTableIndex];
            virtualArray->pm->valids[ptReplace.pmpage_index]= 1;

            int validPMIndex =getValidpage_index();
            virtualArray->pr.page_indexPM = validPMIndex;

            ptReplace.page_index=page_index;
            ptReplace.pmpage_index=validPMIndex;

            auto it = pt.queue.begin();
            pt.queue.erase(it);
            pt.queue.push_back(page_index);

            writeDisk(ptReplace.pmpage_index);

            pt.table[pageTableIndex]= ptReplace;
    }
    else if (pageReplacement.compare("SC") == 0)
    {

            int replaceIndex = pt.queue.at(0), pageTableIndex = 0;
            for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                if (replaceIndex == pt.table[pageTableIndex].page_index)
                    break;

            PageTableEntry ptReplace = pt.table[pageTableIndex];
            virtualArray->pm->valids[ptReplace.pmpage_index]=1;

            int validPMIndex = getValidpage_index();
            virtualArray->pr.page_indexPM = validPMIndex;

            auto it = pt.queue.begin();

            if (ptReplace.referenced)
            {
                replaceIndex = pt.queue.at(1);
                ptReplace.referenced=0;
                ++it;
            }
            for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                if (replaceIndex == pt.table[pageTableIndex].page_index)
                    break;

            pt.queue.erase(it);
            pt.queue.push_back(page_index);
            ptReplace.page_index=page_index;
            ptReplace.pmpage_index=validPMIndex;

             writeDisk(ptReplace.pmpage_index);

             pt.table[pageTableIndex]= ptReplace;

    }
    else if (pageReplacement.compare("LRU") == 0)
     {

            int replaceIndex = pt.queue.at(0), pageTableIndex = 0;
            for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                if (replaceIndex == pt.table[pageTableIndex].page_index)
                    break;
            PageTableEntry ptReplace = pt.table[pageTableIndex];
            virtualArray->pm->valids[ptReplace.pmpage_index]= 1;

            int validPMIndex =getValidpage_index();
            virtualArray->pr.page_indexPM = validPMIndex;

           writeDisk(ptReplace.pmpage_index);

            ptReplace.page_index=page_index;
            ptReplace.pmpage_index=validPMIndex;

            auto it = pt.queue.begin();
            pt.queue.erase(it);
            pt.queue.push_back(page_index);

            pt.table[pageTableIndex]= ptReplace;


    }
    else if (pageReplacement.compare("WSCLOCK") == 0)
     {

                int i = pt.queue.size()-1, pageTableIndex = 0, replaceIndex = 0;

                for (; i >= 0; --i)
                {
                    replaceIndex = pt.queue.at(i);
                    for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                        if (replaceIndex == pt.table[pageTableIndex].page_index)
                            break;
                    if (pt.table[pageTableIndex].referenced)
                    {
                        pt.table[pageTableIndex].referenced=0;
                    }
                    else
                    {
                        virtualArray->pm->valids[pt.table[pageTableIndex].pmpage_index]= 1;
                        int validPMIndex = getValidpage_index();
                        virtualArray->pr.page_indexPM = validPMIndex;

                        pt.table[pageTableIndex].page_index=page_index;
                        pt.table[pageTableIndex].pmpage_index=validPMIndex;
                        auto it = pt.queue.begin() + i;
                        pt.queue.erase(it);
                        pt.queue.push_back(page_index);

                        replaceIndex = pt.queue.at(pt.queue.size()-1);
                        for (pageTableIndex = 0; pageTableIndex < pt.tableSize; ++pageTableIndex)
                            if (replaceIndex == pt.table[pageTableIndex].page_index)
                                break;
                        pt.table[pageTableIndex].referenced=1;
                        break;
                    }
                }


     }
    else
    {
        cerr << "Error: Unacceptable replacement algorithm!" << endl;
        exit(EXIT_FAILURE);
    }
}



void writeDisk(int page_index)
{
    int i = 0;
    for (; i < virtualArray->pr.pm->frameSize; ++i)
         virtualArray->pr.diskFile << virtualArray->pr.pm->arr[page_index][i] << ", ";
     virtualArray->pr.diskFile << endl << endl;
    virtualArray->pr.pm->valids[page_index]=1;
}

int getValidpage_index()
{
    if (virtualArray->pm->physicalPageCounter != virtualArray->pm->numPhysical)
        return virtualArray->pm->physicalPageCounter++;

    int i = 0;
    for (; i < virtualArray->pm->numPhysical; ++i)
        if (virtualArray->pm->valids[i])
            return i;

    return -1;
}

PhysicalMemory::~PhysicalMemory()
{
    if (this->arr != NULL)
    {
        for (int i = 0; i < this->numPhysical; ++i)
            delete [] this->arr[i];

        delete [] this->arr;
        delete [] this->valids;
    }
}

PhysicalMemory::PhysicalMemory(int frameSize, int numPhysical)
{
    this->numPhysical = pow(2, numPhysical);
    this->frameSize = frameSize;
    this->physicalMemorySize = this->frameSize * this->numPhysical;
    this->valids = new int[this->numPhysical];
    this->arr = new int*[this->numPhysical];
    for (int i = 0; i < this->numPhysical; ++i)
    {
        this->arr[i] = new int [this->frameSize];
        this->valids[i] = 1;
    }
}

extern VirtualMemory* virtualArray;
mutex pageTableMutex;

void set(unsigned int index, int value, char * tName)
{
    if (!control(index, tName))
        return;

    int Page = index / virtualArray->frameSize;
    int page_index = index % virtualArray->frameSize;

    PageTableSet(Page, page_index, value);
}

int get(unsigned int index, char * tName)
{
    if (!control(index, tName))
        return -1;


    int Page = index / virtualArray->frameSize;
    int page_index = index % virtualArray->frameSize;

    int returnValue = PageTableGet(Page, page_index);

    return returnValue;
}

Sorter::Sorter(VirtualMemory* virtualArray)
{
    this->virtualArray = virtualArray;
    this->halfSize = virtualArray->numVirtual / 2;
}

Search::Search(VirtualMemory* virtualArray)
{
    this->virtualArray = virtualArray;
    this->halfSize = virtualArray->numVirtual / 2;
}

void bubbleSortAlgorithm(int begin, int end)
{
    int i = begin, j = begin, x = 0, y = 0;

    for (i = 0; i < (end - begin); ++i)
    {
        for (j = begin; j < (end - i - 1); ++j)
        {
            x = get(j, (char*)sort->bubbleString.c_str()); // list[j]
            y = get(j+1, (char*)sort->bubbleString.c_str()); // list[j+1]
            if (x > y)
            {
                set(j, y, (char*)sort->bubbleString.c_str());
                set(j+1, x, (char*)sort->bubbleString.c_str());
            }
        }
    }
}

void bubbleSort()
{
    int i = 0, begin = 0, end = sort->halfSize;
    int frameMult = virtualArray->frameSize;
    int beginCounter = 0, endCounter = frameMult;
    for (i = begin; i < end; ++i)
    {
        clock_t t;
        t = clock();
        pageTableMutex.lock();

        bubbleSortAlgorithm(beginCounter, endCounter);

        pageTableMutex.unlock();

        beginCounter += frameMult;
        endCounter += frameMult;
        t = clock() - t;
        double time_taken = ((double)t)/CLOCKS_PER_SEC;
    }
}


void quickSortAlgorithm(int low, int high)
{
    int i = 0, j = 0, pivot = 0, temp = 0, listPivot = 0, listI = 0, listJ = 0;

    if(low < high)
    {
        pivot = low;
        i = low;
        j = high;

        while(i < j)
        {
            listI = get(i, (char*)sort->quickString.c_str());
            listPivot = get(pivot, (char*)sort->quickString.c_str());
            while(listI <= listPivot && i < high)
            {
                ++i;
                listI = get(i, (char*)sort->quickString.c_str());
            }

            listJ = get(j, (char*)sort->quickString.c_str());
            while(listJ > listPivot)
            {
                --j;
                listJ = get(j, (char*)sort->quickString.c_str());
            }

            if(i < j)
            {
                temp = listI;
                listI = listJ;
                set(i, listI, (char*)sort->quickString.c_str());
                listJ = temp;
                set(j, listJ, (char*)sort->quickString.c_str());
            }
        }
        temp = listPivot;
        listPivot = listJ;
        set(pivot, listPivot, (char*)sort->quickString.c_str());
        listJ = temp;
        set(j, listJ, (char*)sort->quickString.c_str());

        quickSortAlgorithm(low, j-1);
        quickSortAlgorithm(j+1, high);
    }
}

void quickSort()
{
    int i = 0, begin = sort->halfSize, end = sort->halfSize * 2;
    int frameMult = virtualArray->frameSize;
    int beginCounter = begin * frameMult, endCounter = begin * frameMult + frameMult;
    for (i = begin; i < end; ++i)
    {
        clock_t t;
        t = clock();
    pageTableMutex.lock();

        quickSortAlgorithm(beginCounter, endCounter-1);
    pageTableMutex.unlock();
        beginCounter += frameMult;
        endCounter += frameMult;
        t = clock() - t;
        double time_taken = ((double)t)/CLOCKS_PER_SEC;
    }
}

void* LinearSearch(void* arg)
{
     VirtualMemory * virt = (VirtualMemory *) arg;
   virt->pt.initTable( virt->virtualMemorySize);
    int num = current_thread++;
  
    for (int i = num * (virt->virtualMemorySize / 4); 
         i < ((num + 1) * (virt->virtualMemorySize / 4)); i++) 
    {
        if (get(i, (char*)sort->linearString.c_str()) == key)
            f = 1;
    }
}
void* binarySearch(void* arg)
{
   VirtualMemory * virt = (VirtualMemory *) arg;
   virt->pt.initTable( virt->virtualMemorySize);
    int thread_part = part++;
    int mid;
  
    int low = thread_part * ( virt->virtualMemorySize / 4);
    int high = (thread_part + 1) * ( virt->virtualMemorySize / 4);
  
    while (low < high && !found)  {
  
        mid = (high - low) / 2 + low;
  
        if (get(mid, (char*)sort->binaryString.c_str()) == key)  {
            found = true;
            break;
        }
  
        else if (get(mid, (char*)sort->binaryString.c_str()) > key)
            high = mid - 1;
        else
            low = mid + 1;
    }
}

void merge(int low, int mid, int high)
{
    int* left = new int[mid - low + 1];
    int* right = new int[high - mid];
  
    int n1 = mid - low + 1, n2 = high - mid, i, j;
  
    for (i = 0; i < n1; i++)
        left[i] = a[i + low];
  
    for (i = 0; i < n2; i++)
        right[i] = a[i + mid + 1];
  
    int k = low;
    i = j = 0;
    while (i < n1 && j < n2) {
        if (left[i] <= right[j])
            a[k++] = left[i++];
        else
            a[k++] = right[j++];
    }
  
    while (i < n1) {
        a[k++] = left[i++];
    }
  
    while (j < n2) {
        a[k++] = right[j++];
    }
}
  


PageTableEntry::PageTableEntry(PageTableEntry& pte)
{
    this->page_index = pte.page_index;
    this->pmpage_index = pte.pmpage_index;
    this->modified = pte.modified;
    this->referenced = pte.referenced;
}

void PageTable::initTable(int max)
{
    this->tableSize=max;
    this->table = new PageTableEntry[this->tableSize];
}

PageTable::~PageTable()
{
    delete [] this->table;
}

void getArguments(char const *argv[], int* frameSize, int* numPhysical, int* numVirtual, int* pageTablePrintInt,
    string* diskFileName, string* pageReplacement, string* allocPolicy);

int main(int argc, char const *argv[])
{
    if (argc != 8)
    {
        cerr << "Unacceptable arguments!" << endl;
        cerr << "Usage : ./sortArrays frameSize numPhysical numVirtual pageReplacement allocPolicy pageTablePrintInt diskFileName.dat" << endl;
        exit(EXIT_FAILURE);
    }

    int frameSize = 0, numPhysical = 0, numVirtual = 0, pageTablePrintInt = 0;
    string diskFileName = "", pageReplacement = "", allocPolicy = "";

    getArguments(argv, &frameSize, &numPhysical, &numVirtual, &pageTablePrintInt, &diskFileName, &pageReplacement, &allocPolicy);

    virtualArray = new VirtualMemory(numVirtual, numPhysical, frameSize, pageTablePrintInt, allocPolicy, pageReplacement, diskFileName);

    pthread_t fillThreadID;
    pthread_create(&fillThreadID, NULL, fill, (void *) &virtualArray);
    void * fillThreadReturnValue;
    pthread_join(fillThreadID, &fillThreadReturnValue);

    sort =new Sorter(virtualArray);

    thread bubbleThread ([&]{ bubbleSort(); });
    thread quickThread ([&]{ quickSort(); });
  

    bubbleThread.join();
    quickThread.join();

   

    virtualArray->pr.diskFile.close();

   if(sortControl())
   {
        cout<<"Correctly Sorted"<<endl;
   }
   else
   {
        cout<<"Başaramadım"<<endl;
   }
   int k=0;

   for (int i=0; i < (virtualArray->numVirtual/2) ; ++i)
     {
        for (int j = 0; j < virtualArray->frameSize-1; ++j)
        {
                a[k]=virtualArray->arr[i][j];
                k++;
        

        }
     }   
    for (int i = virtualArray->numVirtual/2; i < (virtualArray->numVirtual); ++i)
    {
        for (int j = 0; j < virtualArray->frameSize-1; ++j)
        {
                 a[k]=virtualArray->arr[i][j];
                k++;

        }
            
    }
            

    merge(0, (k / 2 - 1) / 2, k / 2 - 1);
    merge(k / 2, k/2 + (k-1-k/2)/2, k - 1);
    merge(0, (k - 1)/2, k - 1);

   printStatistics();

pthread_t linearThreadID;
    pthread_create(&linearThreadID, NULL, LinearSearch, (void *) &virtualArray);
    void * linearThreadReturnValue;
    pthread_join(linearThreadID, &linearThreadReturnValue);

    pthread_t binaryThreadID;
    pthread_create(&binaryThreadID, NULL, binarySearch, (void *) &virtualArray);
    void * binaryThreadReturnValue;
    pthread_join(binaryThreadID, &binaryThreadReturnValue);



 
    delete virtualArray;

    return 0;
}

void getArguments(char const *argv[], int* frameSize, int* numPhysical, int* numVirtual, int* pageTablePrintInt, string* diskFileName, string* pageReplacement, string* allocPolicy)
{
    sscanf(argv[1],"%d",frameSize);
    sscanf(argv[2],"%d",numPhysical);
    sscanf(argv[3],"%d",numVirtual);

    string temp1(argv[4]);
    *pageReplacement = temp1;

    sscanf(argv[6],"%d",pageTablePrintInt);

    string temp3(argv[7]);
    *diskFileName = temp3;

    if (*frameSize < 0 || *numPhysical < 0 || *numVirtual < 0 || *pageTablePrintInt < 0)
    {
        cerr << "Error : Not positive numbers detected!" << endl;
        exit(EXIT_FAILURE);
    }

    if (pageReplacement->compare("FIFO") != 0 && pageReplacement->compare("LRU") != 0 &&
        pageReplacement->compare("NRU") != 0 && pageReplacement->compare("SC") != 0 && pageReplacement->compare("WSCLOCK") != 0)
    {
        cerr << "Error : Not FIFO, LRU, NRU, SC or WSCLOCK page replacement detected!" << endl;
        exit(EXIT_FAILURE);
    }
}