#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <cstring>


#define COUNT_DOCID 2000
#define SIZE_DOCID  8
#define SIZE_WORDID 8
#define SIZE_OFFSET 8
#define SIZE_N      4

using namespace std;

void
saveIndexFile(const char *name, map<uint64_t, set<uint64_t>> &dict_wordid, set<uint64_t> &set_wordid)
{
    FILE *out_file = fopen(name, "wb");
    for (auto iter_dict = dict_wordid.begin(); iter_dict != dict_wordid.end(); iter_dict++) {
        fwrite(&(iter_dict->first), SIZE_WORDID, 1, out_file);
        set_wordid.insert(iter_dict->first);

        auto set_docids = iter_dict->second;
        uint32_t size_set = set_docids.size();

        fwrite(&size_set, SIZE_N, 1, out_file);
        for(auto iter_set = set_docids.begin(); iter_set != set_docids.end(); iter_set++) {
            fwrite(&(*iter_set), SIZE_DOCID, 1, out_file);
        }
    }
    fclose(out_file);
}

string
createName(const char *name, unsigned long long number)
{
    stringstream ss;
    ss.str(string());
    ss.clear();
    ss << name << number;
    string file_name = ss.str();
    return file_name;
}


pair<unsigned long long, set<uint64_t>>
createIndexsFiles(const char *name_input, const char *name_output)
{
    FILE *file = fopen(name_input, "rb");
    uint64_t docid;
    uint64_t *wordid = nullptr;
    uint32_t n;

    map<uint64_t, set<uint64_t>> dict_wordid = map<uint64_t, set<uint64_t>>();
    set<uint64_t> set_wordid = set<uint64_t>();
    unsigned long long cur_count = 0;
    unsigned long long number_file = 0;

    while (1) {
        if (fread(&docid, SIZE_DOCID, 1, file) != 1)
            break;
        fread(&n, SIZE_N, 1, file);
        wordid = (uint64_t *)realloc(wordid, n * SIZE_WORDID);
        fread(wordid, SIZE_WORDID, n, file);
        cur_count++;
        for(uint32_t i = 0; i < n; i++) {
            dict_wordid[wordid[i]].insert(docid);
        }

        if (cur_count == COUNT_DOCID) {
            const string & name = createName(name_output, number_file);
            saveIndexFile(name.c_str(), dict_wordid, set_wordid);
            dict_wordid = map<uint64_t, set<uint64_t>>();
            cur_count = 0;
            number_file += 1;
        }

    }
    if (dict_wordid.size() > 0) {
        string name = createName(name_output, number_file);
        saveIndexFile(name.c_str(), dict_wordid, set_wordid);
        cur_count = 0;
        number_file += 1;
    }
    free(wordid);
    return pair<unsigned long long, set<uint64_t>>(number_file, set_wordid);
}

void
saveWordids(FILE *out_file, set<uint64_t> set_wordid)
{
    uint64_t offset = 0;
    for(auto iter = set_wordid.begin(); iter != set_wordid.end(); iter++) {
        fwrite(&(*iter), SIZE_WORDID, 1, out_file);
        fwrite(&offset, SIZE_OFFSET, 1, out_file);
    }
    fwrite(&offset, SIZE_OFFSET, 1, out_file);
    fwrite(&offset, SIZE_OFFSET, 1, out_file);
}
void
saveOffsets(FILE *out_file, map<uint64_t, uint64_t> offsets)
{
    fseek(out_file, 0,  SEEK_SET);
    for(auto iter = offsets.begin(); iter != offsets.end(); iter++) {
        fseek(out_file, SIZE_WORDID, SEEK_CUR);
        fwrite(&(iter->second), SIZE_OFFSET, 1, out_file);
    }
}

void
removeIndexFiles(const char *name_input, unsigned long long cnt_index_file)
{
    for (unsigned long long i = 0; i < cnt_index_file; i++) {
        string name = createName(name_input, i);
        remove(name.c_str());
    }

}

void
mergeIndex(const char *name_input, const char *name_output,
                unsigned long long cnt_index_file, set<uint64_t> set_wordid)
{
    FILE *out_file = fopen(name_output, "wb");
    saveWordids(out_file, set_wordid);
    vector<FILE *> in_files = vector<FILE *>();

    vector <uint64_t> wordids = vector <uint64_t>();
    vector <uint64_t *> docids = vector <uint64_t *>();
    vector <uint32_t> counts =  vector <uint32_t>();
    uint64_t max_wordid = *(set_wordid.rbegin()) + 1;
    for(unsigned long long i = 0; i < cnt_index_file; i++) {
        const string &name = createName(name_input, i);
        in_files.push_back(fopen(name.c_str(), "rb"));
    }
    uint64_t cur_wordid;
    uint64_t *cur_docids = nullptr;
    uint32_t cur_n;
    for(auto iter = in_files.begin(); iter != in_files.end(); iter++) {
        fread(&cur_wordid, SIZE_WORDID, 1, *iter);
        fread(&cur_n, SIZE_N, 1, *iter);
        cur_docids = (uint64_t *) malloc(cur_n * SIZE_DOCID);
        fread(cur_docids, SIZE_DOCID, cur_n, *iter);
        wordids.push_back(cur_wordid);
        counts.push_back(cur_n);
        docids.push_back(cur_docids);
    }
    map<uint64_t, uint64_t> offsets =  map<uint64_t, uint64_t>();
    while(1) {
        uint64_t min_value = max_wordid;
        vector<int> indexs = vector<int>();
        for(auto iter = wordids.begin(); iter != wordids.end(); iter++) {
            if (*iter < min_value) {
                indexs.clear();
                indexs.push_back(iter - wordids.begin());
                min_value = *iter;
            }
            else if (*iter == min_value) {
                indexs.push_back(iter - wordids.begin());
            }
        }
        if (min_value == max_wordid)
            break;
        uint32_t all_counts = 0;
        for(auto iter = indexs.begin(); iter != indexs.end(); iter++)
            all_counts += counts[*iter];

        uint64_t *all_docids = (uint64_t *)malloc(all_counts * SIZE_DOCID);
        uint32_t cur_count = 0;
        for(auto iter = indexs.begin(); iter != indexs.end(); iter++) {
            memcpy(all_docids + cur_count, docids[*iter], counts[*iter] * SIZE_DOCID);
            cur_count += counts[*iter];
            if (fread(&(wordids[*iter]), SIZE_WORDID, 1, in_files[*iter]) == 1) {
                fread(&(counts[*iter]), SIZE_N, 1, in_files[*iter]);
                docids[*iter]  = (uint64_t *)realloc(docids[*iter], counts[*iter] * SIZE_DOCID);
                fread(docids[*iter], SIZE_DOCID, counts[*iter], in_files[*iter]);
            } else {
                wordids[*iter] = max_wordid;
                free(docids[*iter]);
            }
        }
        sort(all_docids,all_docids+all_counts);
        offsets[min_value] = ftell(out_file);
        fwrite(&all_counts, SIZE_N, 1, out_file);
        fwrite(all_docids, SIZE_WORDID, all_counts, out_file);
        free(all_docids);
    }
    saveOffsets(out_file, offsets);
    removeIndexFiles(name_input, cnt_index_file);
    fclose(out_file);
}

void
readIndex(const char *name)
{
    FILE *file = fopen(name, "rb");
    map<uint64_t, uint64_t> offsets =  map<uint64_t, uint64_t>();
    uint64_t wordid, offset;
    uint32_t n;
    uint64_t *docid = nullptr;
    while (1) {
        fread(&wordid, SIZE_WORDID, 1, file);
        fread(&offset, SIZE_OFFSET, 1, file);
        if (wordid == 0)
            break;
        offsets[wordid] = offset;
    }
    int k = 0;
    for(auto iter = offsets.begin(); iter != offsets.end(); iter++) {
        if (++k > 10)
            break;
        cout << "wordid: " << iter->first << " offset: " << iter->second << endl;
        fseek(file, iter->second, SEEK_SET);
        fread(&n, SIZE_N, 1, file);
        cout << "count: " << n << " wordids: ";
        docid = (uint64_t *)realloc(docid, n * SIZE_DOCID);
        fread(docid, SIZE_DOCID, n, file);
        for(uint32_t j = 0; j < n; j++)
            cout << docid[j] << ' ';
        cout << endl;
    }
}

int
main(int argc, char *argv[])
{
    if (argc > 2) {
        cout << "Error: enter 1 parametr name_file" << endl;
        return 0;
    }
    const char *name_file = argv[1];
    const char *name_index_files = "index";
    const char *name_full_index = "index";
    pair<unsigned long long, set<uint64_t>> counts;
    counts = createIndexsFiles(name_file, name_index_files);
    mergeIndex(name_index_files, name_full_index, counts.first, counts.second);
    //readIndex(name_full_index);
    return 0;
}
