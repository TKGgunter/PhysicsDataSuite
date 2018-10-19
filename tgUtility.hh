#ifndef tgUtility_h
#define tgUtility_h

#include "BaconAna/DataFormats/interface/TMuon.hh"
#include "BaconAna/DataFormats/interface/TElectron.hh"
#include "BaconAna/DataFormats/interface/TPhoton.hh"
#include "BaconAna/DataFormats/interface/TJet.hh"
#include "BaconAna/DataFormats/interface/TGenParticle.hh"
#include "BLT_II/BLTAnalysis/interface/TGPhysicsObject.hh"
#include <TVector2.h>
#include <TStopwatch.h>
#include <iostream>
#include <string>
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <list>
#include <map>




template <class T>
T* get_value(std::map<std::string, T>* m, std::string name){
		if (m->count(name) > 0 ){
				return &(m->find(name)->second);
		}
		else{
				printf("\e[31mKey %s not found\e[0m\n", name.c_str());
				exit(EXIT_FAILURE);
		}
};


//NOTE General Use Macros makes C++ tolerable
#define FOR_IN(it_name, list)\
		for(auto it_name = list.begin(); it_name != list.end(); it_name++)\
\

#define FOR_IN_fARR(it_name, fArr, type)\
		type* it_name;\
		int _i;\
		for( _i = 0, it_name = (type*) fArr->At(0); _i < fArr->GetEntries(); _i++, it_name=(type*) fArr->At(_i))\
\



#define ENUMERATE_IN(i, it_name, list)\
		int i=0;\
		for(auto it_name = list.begin(); it_name != list.end(); it_name++, i++)\
\

#define ENUMERATE_IN_fARR(i, it_name, fArr, type)\
		type* it_name;\
		int i;\
		for( i = 0, it_name = (type*) fArr->At(0); i < fArr->GetEntries(); i++, it_name=(type*) fArr->At(i))\
\


#define OPEN_TFILE(str_path_name, file)\
		TFile* file = new TFile(str_path_name.c_str(), "OPEN");														\
		if(file->IsZombie()){																													\
				printf("\e[31mFile  %s  Could Not be Found \e[0m\n", str_path_name.c_str());\
				exit(EXIT_FAILURE);																												\
		}																																							\
				


struct ProgressBar{
		long int epoch = 10;
		long int max_epochs = 0;
		long int prev_completed_events = 0;
		long int completed_events = 0;
		long int epoch_completed = 0;
		double average_time_per_epoch = 0.0;
    TStopwatch timer;
};

void progress_print(ProgressBar * progressbar);
void progress_update( ProgressBar * progressbar); 




void testFunc(std::string testStr, float testNumb = -99 );

//=======================================
float qtCalc( TGPhysObject* obj1, TGPhysObject* lep2 );

//=========================================

float deltaPhi(float dPhi_);

float dR(baconhep::TMuon* lep1, baconhep::TJet* jet);

float dR(baconhep::TElectron* lep1, baconhep::TJet* jet);   

float dR( TGPhysObject* obj1, TGPhysObject* obj2);
float dR( TGPhysObject* obj1, baconhep::TJet* obj2);

float dR( float phi1, float phi2, float eta1, float eta2);


void tgSort(std::vector<TGPhysObject>& objList);

//=======================================

void tgCleanVector(std::vector<TGPhysObject>& muonList);


#endif
