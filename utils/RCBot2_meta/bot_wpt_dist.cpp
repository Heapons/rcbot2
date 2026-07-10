// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "engine_wrappers.h"

#include "bot_wpt_dist.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_compress.h"

#include <cstdlib>
#include <cstring>

typedef struct
{
	int version;
	int numwaypoints;
	int maxwaypoints;
}wpt_dist_hdr_t;

int CWaypointDistances::m_Distances[CWaypoints::MAX_WAYPOINTS][CWaypoints::MAX_WAYPOINTS];
float CWaypointDistances::m_fSaveTime = 0.0f;

void CWaypointDistances::load()
{
	wpt_dist_hdr_t hdr;
	const char* szMapName = CBotGlobals::getMapName();

	if (szMapName && *szMapName)
	{
		char filename[1024];
		CBotGlobals::buildFileName(filename, szMapName, BOT_AUXILERY_FOLDER, BOT_WAYPOINT_DST_EXTENSION, true);

		constexpr std::size_t totalSize = sizeof(wpt_dist_hdr_t) + sizeof(m_Distances);
		unsigned char* pBuf = static_cast<unsigned char*>(std::malloc(totalSize));

		if (!pBuf)
			return;

		if (!RCBot_CompressedLoad(filename, pBuf, totalSize))
		{
			std::free(pBuf);
			return;
		}

		// pBuf is malloc'd as totalSize = sizeof(wpt_dist_hdr_t) + sizeof(m_Distances), so copying
		// out just the header is in-bounds. V1086's "underflow" is a false positive. [APG]RoboCop[CL]
		std::memcpy(&hdr, pBuf, sizeof(wpt_dist_hdr_t)); //-V1086

		if ((hdr.maxwaypoints == CWaypoints::MAX_WAYPOINTS) && (hdr.numwaypoints == CWaypoints::numWaypoints()) && (hdr.version == WPT_DIST_VER))
		{
			std::memcpy(m_Distances, pBuf + sizeof(wpt_dist_hdr_t), sizeof(m_Distances));
		}

		std::free(pBuf);
		m_fSaveTime = engine->Time() + 100.0f;
	}
}

void CWaypointDistances::save()
{
	const char* szMapName = CBotGlobals::getMapName();

	if (szMapName && *szMapName)
	{
		char filename[1024];
		wpt_dist_hdr_t hdr;

		CBotGlobals::buildFileName(filename, szMapName, BOT_AUXILERY_FOLDER, BOT_WAYPOINT_DST_EXTENSION, true);

		hdr.maxwaypoints = CWaypoints::MAX_WAYPOINTS;
		hdr.numwaypoints = CWaypoints::numWaypoints();
		hdr.version = WPT_DIST_VER;

		constexpr std::size_t totalSize = sizeof(wpt_dist_hdr_t) + sizeof(m_Distances);
		unsigned char* pBuf = static_cast<unsigned char*>(std::malloc(totalSize));

		if (!pBuf)
		{
			m_fSaveTime = engine->Time() + 100.0f;
			return;
		}

		std::memcpy(pBuf, &hdr, sizeof(wpt_dist_hdr_t));
		std::memcpy(pBuf + sizeof(wpt_dist_hdr_t), m_Distances, sizeof(m_Distances));

		CBotGlobals::makeFolders(filename);
		RCBot_CompressedSave(filename, pBuf, totalSize);

		std::free(pBuf);
		m_fSaveTime = engine->Time() + 100.0f;
	}
	//}
}

float CWaypointDistances::getDistance(const int iFrom, const int iTo)
{
	if (m_Distances[iFrom][iTo] == -1)
	{
		CWaypoint* waypointFrom = CWaypoints::getWaypoint(iFrom);
		CWaypoint* waypointTo = CWaypoints::getWaypoint(iTo);

		if (waypointFrom == nullptr || waypointTo == nullptr)
		{
			return -1.0f;
		}

		return (waypointFrom->getOrigin() - waypointTo->getOrigin()).Length();
	}
	return static_cast<float>(m_Distances[iFrom][iTo]);
}