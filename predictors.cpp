#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

struct input{
  string addr, behavior, taddr;
};

vector<input> readFile(string filename) {
    vector<input> store;
    ifstream inFile(filename);
    if (inFile.is_open()) {
      string addr, behavior, taddr;
      while (inFile >> addr >> behavior >> taddr) {
          input l;
          l.addr = addr;
          l.behavior = behavior;
          l.taddr = taddr;
          store.push_back(l);
      }
      inFile.close();
    } else {
      cerr << "File " << filename << " doesn't exist" << endl;
    }
    return store;
}

int alwaysTakenPredictor(vector<input> store){
    int c = 0;
    for(int i = 0; i < store.size(); i++){
        if(store[i].behavior == "T"){
            c++;
        }
    }
    return c;
}

int alwaysNotTakenPredictor(vector<input> store){
    int c = 0;
    for(int i = 0; i < store.size(); i++){
        if(store[i].behavior == "NT"){
            c++;
        }
    }
    return c;
}

int singleBitBimodalPredictor(vector<input> store, int tableSize, bool initial){
  vector<bool> table(tableSize, initial);
  int c = 0;

  for(int i = 0; i < store.size(); i++){
    unsigned long long addr = stoull(store[i].addr, nullptr, 16);
    int index = addr % tableSize;
    bool predict = table[index];
    bool b;
    if(store[i].behavior == "T"){
      b = true;
    } else {
      b = false;
    }

    if(b == predict){
      c++;
    } else {
      table[index] = b;
    }
  }
  return c;
}

int twoBitBimodalPredictor(vector<input> store, int tableSize, bool initial){
  vector<int> table(tableSize, 3);
  int c = 0;

  for(int i = 0; i < store.size(); i++){
    unsigned long long addr = stoull(store[i].addr, nullptr, 16);
    int index = addr % tableSize;
    bool predict = table[index];
    bool b;
    if(store[i].behavior == "T"){
      b = true;
    } else {
      b = false;
    }

    if(b == true && table[index] >= 2){
      if(table[index] == 2)
	table[index]++;
      c++;
    } else if(b == true && table[index] < 2){
      table[index]++;
    } else if(b == false && table[index] >= 2){
      table[index]--;
    } else if(b == false && table[index] < 2){
      if(table[index] == 1)
	table[index]--;
      c++;
    }
  }
  return c;
}

int gsharePredictor(vector<input> store, int hbits){
  vector<int> table(2048, 3);
  int c = 0;
  unsigned long long ghr = 0;

  for(int i = 0; i < store.size(); i++){
    unsigned long long addr = stoull(store[i].addr, nullptr, 16);
    int index = (addr ^ ghr) % 2048;
    bool prediction = table[index];
    bool b;
    if(store[i].behavior == "T"){
      b = true;
    } else {
      b = false;
    }

    if(b == true && table[index] >= 2){
      if(table[index] == 2)
	table[index]++;
      c++;
    } else if(b == true && table[index] < 2){
      table[index]++;
    } else if(b == false && table[index] >= 2){
      table[index]--;
    } else if(b == false && table[index] < 2){
      if(table[index] == 1)
	table[index]--;
      c++;
    }

    ghr = (ghr << 1) & ((1 << hbits) - 1);
    ghr = ghr | (b ? 1:0);
  }
  return c;
}

int tournamentPredictor(vector<input> store){
  vector<int> btable(2048, 3);
  vector<int> gtable(2048, 3);
  vector<int> stable(2048, 0);

  int c = 0;
  int ghr = 0;
  int mask = ((1 << 11) - 1);

  for(int i = 0; i < store.size(); i++){
    unsigned long long addr = stoull(store[i].addr, nullptr, 16);
    int bindex = addr % 2048;
    int gindex = (addr ^ ghr) % 2048;
    int sindex = bindex;

    bool bprediction = (btable[bindex] < 2) ? false : true;
    bool gprediction = (gtable[gindex]) < 2 ? false : true;
    bool b;
    if(store[i].behavior == "T"){
      b = true;
    } else {
      b = false;
    }

    bool sresult = (stable[sindex] < 2) ? gprediction : bprediction;

    if(b == sresult || !b == !sresult){
      c++;
    }
    ghr = ((ghr << 1) | (b ? 1 : 0) & mask);

    if(b && btable[bindex] < 3){
      btable[bindex]++;
    } else if(!b && btable[bindex] > 0){
      btable[bindex]--;
    }

    if(b && gtable[gindex] < 3){
      gtable[gindex]++;
    } else if(!b && gtable[gindex] > 0){
      gtable[gindex]--;
    }

    if(bprediction != gprediction){
      if(b == gprediction && stable[sindex] > 0){
	stable[sindex]--;
      } else if(b == bprediction && stable[sindex] < 3){
	  stable[sindex]++;
      }
    }
    
  }
  return c;
}

int branchTargetBuffer(vector<input> store, int& tpredictions){
  vector<unsigned long long> btb(512, 0x0);
  vector<int> btable(512, 1);

  int c = 0;
  tpredictions = 0;

  for(int i = 0; i < store.size(); i++){
    unsigned long long addr = stoull(store[i].addr, nullptr, 16);
    unsigned long long taddr = stoull(store[i].taddr, nullptr, 16);
    int index = addr % 512;
    bool b;
    if(store[i].behavior == "T"){
      b = true;
    } else {
      b = false;
    }

    if(btable[index] == 1){
      tpredictions++;
      if(btb[index] != 0x0 && btb[index] == taddr){
	c++;
      }
    }

    if(b){
      btable[index] = 1;
      btb[index] = taddr;
    } else {
      btable[index] = 0;
    }
  }
  return c;
}

int main(int argc, char* argv[]){
  if(argc != 3){
    cout << "Please enter correct number of arguments" << endl;
    return -1;
  }

  vector<input> store = readFile(argv[1]);

  FILE* fp = fopen(argv[2], "w");

  int c;
  int total = store.size();

  c = alwaysTakenPredictor(store);
  fprintf(fp, "%d,%d;\n", c, total);
  c = alwaysNotTakenPredictor(store);
  fprintf(fp, "%d,%d;\n", c, total);

  int tableSizes[] = {16, 32, 128, 256, 512, 1024, 2048};

  for(int i: tableSizes){
    c = singleBitBimodalPredictor(store, i, true);
    if(i == 2048){
      fprintf(fp, "%d,%d;\n", c, total);
    } else {
      fprintf(fp, "%d,%d; ", c, total);
    }
  }
  
  for(int i: tableSizes){
    c = twoBitBimodalPredictor(store, i, true);
    if(i == 2048){
      fprintf(fp, "%d,%d;\n", c, total);
    } else {
      fprintf(fp, "%d,%d; ", c, total);
    }
  }

  for(int hbits = 3; hbits <= 11; hbits++){
    c = gsharePredictor(store, hbits);
    if(hbits == 11){
      fprintf(fp, "%d,%d;\n", c, total);
    } else {
      fprintf(fp, "%d,%d; ", c, total);
    }
  }

  c = tournamentPredictor(store);
  fprintf(fp, "%d,%d;\n", c, total);

  int tpredictions;
  c = branchTargetBuffer(store, tpredictions);
  fprintf(fp, "%d,%d;\n", c, tpredictions);
  fclose(fp);

  return 0;
}
