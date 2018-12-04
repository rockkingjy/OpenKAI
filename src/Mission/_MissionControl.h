/*
 * _MissionControl.h
 *
 *  Created on: Aug 27, 2016
 *      Author: Kai
 */

#ifndef OpenKAI_src_Mission__MissionControl_H_
#define OpenKAI_src_Mission__MissionControl_H_

#include "../Base/common.h"
#include "../Base/_ThreadBase.h"
#include "MissionBase.h"
#include "Waypoint.h"

#define ADD_MISSION(x) if(pKM->m_class==#x){M.m_pInst=new x();M.m_pKiss=pKM;}

namespace kai
{

struct MISSION_INST
{
	MissionBase* m_pInst;
	Kiss* m_pKiss;

	void init(void)
	{
		m_pInst = NULL;
		m_pKiss = NULL;
	}
};

class _MissionControl: public _ThreadBase
{
public:
	_MissionControl();
	virtual ~_MissionControl();

	bool init(void* pKiss);
	bool start(void);
	bool draw(void);
	bool console(int& iY);
	int check(void);

	int getMissionIdx(const string& missionName);
	MissionBase* getCurrentMission(void);
	int getCurrentMissionIdx(void);
	string* getCurrentMissionName(void);
	int getLastMissionIdx(void);
	bool transit(const string& nextMissionName);
	bool transit(int nextMissionIdx);

public:
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_MissionControl *) This)->update();
		return NULL;
	}

	vector<MISSION_INST> m_vMission;
	int	m_iMission;

};

}
#endif
