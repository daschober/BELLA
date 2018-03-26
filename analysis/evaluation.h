#include "IntervalTree.h"
#include <omp.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <math.h>
#include <limits.h>
#include <bitset>
#include <unordered_map>
#include <map>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h> 
#include <sstream>
#include <set>
#include <memory>

using namespace std;

struct refInfo {

    string ref;
    string read;
    int start;
    int end;
};

struct readInfo {

    string ref;
    int start;
    int end;
};

typedef vector<refInfo> vectInfo;

int estimeOvl(int colStart, int colEnd, int colLen, int rowStart, int rowEnd, int rowLen)
{
    int diffCol = colEnd-colStart;
    int diffRow = rowEnd-rowStart;
    int minLeft = min(colStart, rowStart);
    int minRight = min(colLen-colEnd, rowLen-rowEnd);

    int result = minLeft+minRight+(diffCol+diffRow)/2;
    return result;
}

/**
 * @brief trueOv omputes the number of true overlapping reads
 * @param truthInfo
 * @param simulated (default false)
 * @param minOvl
 * @return number of true overlapping pairs
 */
double trueOv(map<string,vectInfo> & truthInfo, bool simulated, int minOvl) 
{
    vector<Interval<std::string>> intervals;
    vector<Interval<std::string>> queries;
    vector<Interval<std::string>>::iterator q;
    map<string,vectInfo>::iterator key; // outer iterator
    vectInfo::iterator it; // inner iterator
    double trueOvls = 0;
    //ofstream ofs("current-ev.txt", ofstream::out);

    for(key = truthInfo.begin(); key != truthInfo.end(); ++key)
    {
         for(it = key->second.begin(); it != key->second.end(); ++it)
         {
             intervals.push_back(Interval<string>(it->start, it->end, it->read));
             queries.push_back(Interval<string>(it->start, it->end, it->read));
         }

         IntervalTree<string> tree;
         vector<size_t> treeCount;
         
         tree = IntervalTree<string>(intervals); // reference 
         
         for (q = queries.begin(); q != queries.end(); ++q) // tree search for a given reference
         {
             vector<Interval<string>> results;
             tree.findOverlapping(q->start, q->stop, q->value, results, minOvl);
             treeCount.push_back(results.size());
         }
         
         for(size_t t = 0; t < treeCount.size(); ++t) // count for a given reference
         { 
             trueOvls = trueOvls + (double)treeCount[t];  // cumulative
         }

         intervals.clear();
         queries.clear();
    }  
    //ofs.close();        
    return trueOvls;
}

/**
 * @brief computeLength computes the overlap length between
 * potential overlapping reads pairs
 * @param readMap
 * @param colName
 * @param rowName
 * @return alignment length
 */
//#ifdef MULTIMAPPED
typedef multimap<string,readInfo>::iterator MMAPIterator;
int computeLength(multimap<string,readInfo> & readMap, string & colName, string & rowName) 
{
    int length = 0, maxlength = 0;

    // It returns a pair representing the range of elements with key equal to the one we want
    pair<MMAPIterator, MMAPIterator> colRange = readMap.equal_range(colName);
    pair<MMAPIterator, MMAPIterator> rowRange = readMap.equal_range(rowName);

    // If there are no elements not less than key, past-the-end (see end()) iterator is returned as the first element. 
    // Similarly if there are no elements greater than key, past-the-end iterator is returned as the second element.
    if(colRange.first != readMap.end() && colRange.second != readMap.end() && rowRange.first != readMap.end() && rowRange.second != readMap.end()) // needed as handling real dataset the aligned reads in sam file could be != the original number of reads
    {
        // Iterate over the range
        for (MMAPIterator itC = colRange.first; itC != colRange.second; itC++)
        {
            for (MMAPIterator itR = rowRange.first; itR != rowRange.second; itR++)
            {
                if(itC->second.ref == itR->second.ref)
                {   
                    if(itC->second.start < itR->second.start) 
                    {
                        if(itC->second.end > itR->second.start)
                            length = min((itC->second.end - itR->second.start), (itR->second.end - itR->second.start));
                    }
                else if(itC->second.start > itR->second.start) 
                {
                    if(itR->second.end > itC->second.start)
                        length = min((itR->second.end - itC->second.start), (itC->second.end - itC->second.start));
                } 
                else length = min((itR->second.end - itC->second.start), (itC->second.end - itC->second.start)); 
                }

                if(length > maxlength)
                    maxlength = length; 
            }
        }
    }
    return maxlength;
}
//#else
//int computeLength(unordered_map<string,readInfo> & readMap, string & colName, string & rowName) 
//{
//    int alignment_length = 0;
//
//    unordered_map<string,readInfo>::iterator jit;
//    unordered_map<string,readInfo>::iterator iit;
//
//    jit = readMap.find(colName); // col name
//    iit = readMap.find(rowName); // row name 
//
//    if(iit != readMap.end() && jit != readMap.end()) // needed as handling real dataset the aligned reads in sam file could be != the original number of reads
//    {
//        if(iit->second.ref == jit->second.ref)
//        {   
//            if(iit->second.start < jit->second.start) {
//                if(iit->second.end > jit->second.start)
//                    alignment_length = min((iit->second.end - jit->second.start), (jit->second.end - jit->second.start));
//            }
//            else if(iit->second.start > jit->second.start) 
//            {
//                if(jit->second.end > iit->second.start)
//                    alignment_length = min((jit->second.end - iit->second.start), (iit->second.end - iit->second.start));
//            } 
//            else alignment_length = min((jit->second.end - iit->second.start), (iit->second.end - iit->second.start)); 
//        } 
//    }
//    return alignment_length;
//}
//#endif
/**
 * @brief benchmarkingAl retrives recall/precision values
 * @param groundtruth (input file)
 * @param bella (file containing bella alignments)
 * @param minimap (file containing minimap overlaps)
 * @param mhap (file containing mhap overlaps)
 * @param blasr (file containing blasr alignments)
 * @param daligner (file containing daligner alignments)
 * @param simulated (default false)
 * @param minOvl
 */
void benchmarkingAl(ifstream & groundtruth, ifstream & bella, ifstream & minimap, ifstream & mhap, 
    ifstream & blasr, ifstream & daligner, bool simulated, int minOvl) // add blasr && daligner && mhap && bella
{
    map<string,vectInfo> isInThere;
    map<string,vectInfo>::iterator iter;
    multimap<string,readInfo> readMap;
    map<pair<string,string>, bool> checkBella;
    map<pair<string,string>, bool> checkMinimap;
    map<pair<string,string>, bool> checkMhap;
    map<pair<string,string>, bool> checkBlasr;
    map<pair<string,string>, bool> checkDaligner;
    map<pair<string,string>, bool>::iterator it;
    
    int alignment_length;
    
    double ovlsbella = 0, truebella = 0;
    double ovlsminimap = 0, trueminimap = 0;
    double ovlsmhap = 0, truemhap = 0;
    double ovlsblasr = 0, trueblasr = 0;
    double ovlsdal = 0, truedal = 0;

    cout << "\nbuilding the ground truth" << endl;

    if(simulated)
    {
        if(groundtruth.is_open())
        {
            string ref;
            string prev;
            string read;
            string dontcare1;
            string dontcare2;
            refInfo ovlInfo;
            readInfo perRead;
            int start;
            int end;

            while(groundtruth >> ref >> start >> end >> read >> dontcare1 >> dontcare2)
            {
                perRead.ref = ref;
                perRead.start = start;
                perRead.end = end;
                readMap.insert(make_pair("@"+read,perRead));

                ovlInfo.ref = ref;
                ovlInfo.read = "@" + read;
                ovlInfo.start = start;
                ovlInfo.end = end;
                
                iter = isInThere.find(ref);
                if(iter == isInThere.end())
                {
                    vectInfo temp;
                    temp.push_back(ovlInfo); // all the element of a chromosome
                    isInThere.insert(map<string,vectInfo>::value_type(ref,temp));
                }
                else
                {
                    iter->second.push_back(ovlInfo);
                    isInThere[ref] = iter->second;
                }
            }
            //isInThere.push_back(vectOf); // insert the last chromosome
            cout << "num reads: " << readMap.size() << endl;
            cout << "num sub-references: " << isInThere.size() << endl;
            //for(int i = 0; i < truthInfo.size(); ++i)
            //    cout << "size chromosome: " << truthInfo.at(i).size() << endl;

        } else cout << "Error opening the ground truth file" << endl;
    }
    else // from sam file
    {
        if(groundtruth.is_open())
        {
            refInfo ovlInfo;
            readInfo perRead;
            string prev;
            string read;
            string ref;
            int start;
            int end;
    
            while(groundtruth >> ref >> read >> start >> end)
            {
                perRead.ref = ref;
                perRead.start = start;
                perRead.end = end;
                readMap.insert(make_pair("@"+read,perRead));

                ovlInfo.ref = ref;
                ovlInfo.read = "@" + read;
                ovlInfo.start = start;
                ovlInfo.end = end;
                
                iter = isInThere.find(ref);
                if(iter == isInThere.end())
                {
                    vectInfo temp;
                    temp.push_back(ovlInfo); // all the element of a chromosome
                    isInThere.insert(map<string,vectInfo>::value_type(ref,temp));
                }
                else
                {
                    iter->second.push_back(ovlInfo);
                    isInThere[ref] = iter->second;
                }
            }
            //isInThere.push_back(vectOf); // insert the last chromosome
            cout << "num reads: " << readMap.size() << endl;
            cout << "num sub-references: " << isInThere.size() << endl;
            //for(int i = 0; i < truthInfo.size(); ++i)
                //cout << "size chromosome: " << truthInfo.at(i).size() << endl;

        } else cout << "Error opening the ground truth file" << endl;
    }
    
    groundtruth.clear();
    groundtruth.seekg(0, ios::beg);
    ofstream truePositivesBella;
    truePositivesBella.open("truepositives-bella.out", std::ofstream::out | std::ofstream::app);

    cout << "computing BELLA recall/precision" << endl;
    if(bella.is_open())
    {
        string line;
        while(getline(bella, line))
        {   
            ovlsbella++;
            
            stringstream lineStream(line);
            string colName, rowName, nkmer, score;
            string colStart, colEnd, colLen, rowStart, rowEnd, rowLen;
            
            getline(lineStream, colName, '\t');
            getline(lineStream, rowName, '\t');
            getline(lineStream, nkmer, '\t');
            getline(lineStream, score, '\t');
            getline(lineStream, colStart, '\t');
            getline(lineStream, colEnd, '\t');
            getline(lineStream, colLen, '\t');
            getline(lineStream, rowStart, '\t');
            getline(lineStream, rowEnd, '\t');
            getline(lineStream, rowLen, '\t');
                                
            if(colName == rowName) // to be sure to not count self aligned pairs
                ovlsbella--;
            else
            {
                it = checkBella.find(make_pair(colName, rowName));
                if(it == checkBella.end())
                {       
                    checkBella.insert(make_pair(make_pair(colName, rowName), true));
                    // Compute the overlap length between potential overlapping reads pairs 
                    alignment_length = computeLength(readMap, colName, rowName);
                    // currently recall > 1
                    if(alignment_length >= minOvl)
                        truebella++;
                    else 
                    {
                        int ovlEstime = estimeOvl(stoi(colStart), stoi(colEnd), stoi(colLen), stoi(rowStart), stoi(rowEnd), stoi(rowLen));
                        if (ovlEstime < minOvl)
                            ovlsbella--;
                    }
                }
            }
        }
    } 
    truePositivesBella.close();

    cout << "computing Minimap recall/precision" << endl;
    if(minimap.is_open())
    {
        string line;
        while(getline(minimap, line))
        {
            ovlsminimap++;
            stringstream lineStream(line);
            string colName, rowName, colStart, colEnd, rowLen, colLen;
            string strand, rowStart, rowEnd;
    
            getline(lineStream, colName, '\t' );
            getline(lineStream, colLen, '\t' );
            getline(lineStream, colStart, '\t' );
            getline(lineStream, colEnd, '\t' );
            getline(lineStream, strand, '\t' );
            getline(lineStream, rowName, '\t' );
            getline(lineStream, rowLen, '\t' );
            getline(lineStream, rowStart, '\t' );
            getline(lineStream, rowEnd, '\t' );
    
            colName = "@" + colName;
            rowName = "@" + rowName;

            if(colName == rowName) // to be sure to not count self aligned pairs
                ovlsminimap--;
            else
            {
                it = checkMinimap.find(make_pair(colName, rowName));
                if(it == checkMinimap.end())
                {
                    checkMinimap.insert(make_pair(make_pair(colName, rowName), true));
                    // Compute the overlap length between potential overlapping reads pairs 
                    alignment_length = computeLength(readMap, colName, rowName);
                    if(alignment_length >= minOvl)
                        trueminimap++;
                    else 
                    {
                        int ovlEstime = estimeOvl(stoi(colStart), stoi(colEnd), stoi(colLen), stoi(rowStart), stoi(rowEnd), stoi(rowLen));
                        if (ovlEstime < minOvl)
                            ovlsminimap--;
                    }
                }
            }
        }
    }

    cout << "computing MHAP sensitive recall/precision" << endl;
    if(mhap.is_open())
    {
        string line;
        while(getline(mhap, line))
        {
            ovlsmhap++;
            stringstream lineStream(line);
            string colName, rowName;

            getline(lineStream, colName, ' ');
            getline(lineStream, rowName, ' ');

            colName = "@" + colName;
            rowName = "@" + rowName;

            if(colName == rowName) // to be sure to not count self aligned pairs
                ovlsmhap--;
            else
            {
                it = checkMhap.find(make_pair(colName, rowName));
                if(it == checkMhap.end())
                {
                    checkMhap.insert(make_pair(make_pair(colName, rowName), true));
                    // Compute the overlap length between potential overlapping reads pairs 
                    alignment_length = computeLength(readMap, colName, rowName);
                    if(alignment_length >= minOvl)
                        truemhap++;
                    //else 
                    //{
                    //    int ovlEstime = estimeOvl(stoi(colStart), stoi(colEnd), stoi(colLen), stoi(rowStart), stoi(rowEnd), stoi(rowLen));
                    //    if (ovlEstime < minOvl)
                    //        ovlsmhap--;
                    //}
                }
            }
        }
    }

    cout << "computing BLASR recall/precision" << endl;
    if(blasr.is_open())
    {
        string line;
        while(getline(blasr, line))
        {
            ovlsblasr++;
            stringstream lineStream(line);
            string colName, rowName;

            getline(lineStream, colName, ' ');
            getline(lineStream, rowName, ' ');

            colName = "@" + colName;
            rowName = "@" + rowName;

            if(colName == rowName) // to be sure to not count self aligned pairs
                ovlsblasr--;
            else
            {
                it = checkBlasr.find(make_pair(colName, rowName));
                if(it == checkBlasr.end())
                {
                    checkBlasr.insert(make_pair(make_pair(colName, rowName), true));
                    // Compute the overlap length between potential overlapping reads pairs 
                    alignment_length = computeLength(readMap, colName, rowName);
                    if(alignment_length >= minOvl)
                        trueblasr++;
                    //else 
                    //{
                    //    int ovlEstime = estimeOvl(stoi(colStart), stoi(colEnd), stoi(colLen), stoi(rowStart), stoi(rowEnd), stoi(rowLen));
                    //    if (ovlEstime < minOvl)
                    //        ovlsblasr--;
                    //}
                }
            }
        }
    }

    cout << "computing DALIGNER recall/precision" << endl;
    if(daligner.is_open())
    {
        string line;
        while(getline(daligner, line))
        {
            ovlsdal++;
            stringstream lineStream(line);
            string colName, rowName;

            getline(lineStream, colName, ' ');
            getline(lineStream, rowName, ' ');

            colName = colName;
            rowName = rowName;

            if(colName == rowName) // to be sure to not count self aligned pairs
                ovlsblasr--;
            else
            {
                it = checkBlasr.find(make_pair(colName, rowName));
                if(it == checkBlasr.end())
                {
                    checkBlasr.insert(make_pair(make_pair(colName, rowName), true));
                    // Compute the overlap length between potential overlapping reads pairs 
                    alignment_length = computeLength(readMap, colName, rowName);
                    if(alignment_length >= minOvl)
                        trueblasr++;
                    //else 
                    //{
                    //    int ovlEstime = estimeOvl(stoi(colStart), stoi(colEnd), stoi(colLen), stoi(rowStart), stoi(rowEnd), stoi(rowLen));
                    //    if (ovlEstime < minOvl)
                    //        ovlsdal--;
                    //}
                }
            }
        }
    }

    groundtruth.clear();
    groundtruth.seekg(0, ios::beg);
    double truetruth = trueOv(isInThere, simulated, minOvl);

    bella.close();
    minimap.close();
    mhap.close();
    blasr.close();
    daligner.close();
    groundtruth.close();

    // Ground Truth 
    cout << "true overlapping from ground truth = " << truetruth << "\n" << endl;
    // BELLA Recall and precision 
    cout << "overlapping from BELLA = " << ovlsbella << endl;
    cout << "true overlapping from BELLA = " << truebella << endl;
    cout << "recall BELLA = " << truebella/truetruth << endl;
    cout << "precision BELLA = " << truebella/ovlsbella << "\n" << endl;
    // Minimap Recall and precision 
    // as -S option count overlaps only once 
    // (A ov B, but not B ov A), while all other 
    // (included the ground truth) count all ovls and the self-ovls
    cout << "overlapping from minimap = " << ovlsminimap << endl;
    cout << "true overlapping from minimap = " << trueminimap << endl;
    cout << "recall minimap = " << (trueminimap*2)/truetruth << endl;
    cout << "precision minimap = " << trueminimap/ovlsminimap << "\n" << endl;
    // MHAP Recall and precision 
    cout << "overlapping from MHAP = " << ovlsmhap << endl;
    cout << "true overlapping from MHAP= " << truemhap << endl;
    cout << "recall MHAP = " << truemhap/truetruth << endl;
    cout << "precision MHAP = " << truemhap/ovlsmhap << "\n" << endl;
    // BLASR Recall and precision 
    cout << "overlapping from BLASR = " << ovlsblasr << endl;
    cout << "true overlapping from BLASR = " << trueblasr << endl;
    cout << "recall BLASR = " << trueblasr/truetruth << endl;
    cout << "precision BLASR = " << trueblasr/ovlsblasr << "\n" << endl;
    // DALIGNER Recall and precision 
    cout << "overlapping from DALIGNER = " << ovlsdal << endl;
    cout << "true overlapping from DALIGNER = " << truedal << endl;
    cout << "recall DALIGNER = " << truedal/truetruth << endl;
    cout << "precision DALIGNER = " << truedal/ovlsdal << "\n" << endl;
}
