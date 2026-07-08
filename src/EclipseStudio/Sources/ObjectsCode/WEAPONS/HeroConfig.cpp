#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "HeroConfig.h"

void HeroConfig::resetMesh()
{
	for(size_t i=0; i<m_LegsMeshes.size(); ++i)
		m_LegsMeshes[i] = NULL;
	for(size_t i=0; i<m_HeadMeshes.size(); ++i)
		m_HeadMeshes[i] = NULL;
	for(size_t i=0; i<m_BodyMeshes.size(); ++i)
		m_BodyMeshes[i] = NULL;
	for(size_t i=0; i<m_BodyMeshesFPS.size(); ++i)
		m_BodyMeshesFPS[i] = NULL;
	for(size_t i=0; i<m_HairMeshes.size(); ++i)
		m_HairMeshes[i] = NULL;
	for(size_t i=0; i<m_FeetMeshes.size(); ++i)
		m_FeetMeshes[i] = NULL;
}

r3dMesh* HeroConfig::getHeadMesh(int index) const
{
	r3d_assert(index>=0 && index<(int)m_HeadMeshes.size());
	if(m_HeadMeshes[index] == 0)
	{
		char tmpBuf[512];
		sprintf(tmpBuf, "%s_Head_%02d.sco", m_BaseModelName, index+1);
		m_HeadMeshes[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
		if(m_HeadMeshes[index]==NULL)
		{
			r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
		}
		r3d_assert(m_HeadMeshes[index]);
	}

	return m_HeadMeshes[index];
}

r3dMesh* HeroConfig::getLegMesh(int index) const
{
	r3d_assert(index>=0 && index < (int)m_LegsMeshes.size());
	if(m_LegsMeshes[index] == 0)
	{
		char tmpBuf[512];
		sprintf(tmpBuf, "%s_Legs_%02d.sco", m_BaseModelName, index+1);
		m_LegsMeshes[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
		if(m_LegsMeshes[index]==NULL)
		{
			r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
		}
		r3d_assert(m_LegsMeshes[index]);
	}

	return m_LegsMeshes[index];
}

r3dMesh* HeroConfig::getBodyMesh(int index, bool isFPS) const
{
	r3d_assert(m_BodyMeshes.size() == m_BodyMeshesFPS.size());
	if(!isFPS)
	{
		r3d_assert(index>=0 && index < (int)m_BodyMeshes.size());
		if(m_BodyMeshes[index] == 0)
		{
			char tmpBuf[512];
			sprintf(tmpBuf, "%s_Body_%02d.sco", m_BaseModelName, index+1);
			m_BodyMeshes[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
			if(m_BodyMeshes[index]==NULL)
			{
				r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
			}
			r3d_assert(m_BodyMeshes[index]);
		}

		return m_BodyMeshes[index];
	}
	else
	{
		r3d_assert(index>=0 && index < (int)m_BodyMeshesFPS.size());
		if(m_BodyMeshesFPS[index] == 0)
		{
			char tmpBuf[512];
			sprintf(tmpBuf, "%s_BodyFPS%02d.sco", m_BaseModelName, index+1);
			if(!r3dFileExists(tmpBuf))
				sprintf(tmpBuf, "%s_BodyFPS_%02d.sco", m_BaseModelName, index+1);
			if(!r3dFileExists(tmpBuf))
				sprintf(tmpBuf, "%s_BodyFPS01.sco", m_BaseModelName);
			if(!r3dFileExists(tmpBuf))
				sprintf(tmpBuf, "%s_BodyFPS_01.sco", m_BaseModelName);
			m_BodyMeshesFPS[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
			if(m_BodyMeshesFPS[index]==NULL)
			{
				r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
			}
			r3d_assert(m_BodyMeshesFPS[index]);
		}

	return m_BodyMeshesFPS[index];
	}
}

r3dMesh* HeroConfig::getHairMesh(int index) const
{
	r3d_assert(index>=0 && index < (int)m_HairMeshes.size());
	if(m_HairMeshes[index] == 0)
	{
		char tmpBuf[512];
		sprintf(tmpBuf, "%s_Hair_%02d.sco", m_BaseModelName, index+1);
		m_HairMeshes[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
		if(m_HairMeshes[index]==NULL)
		{
			r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
		}
		r3d_assert(m_HairMeshes[index]);
	}

	return m_HairMeshes[index];
}

r3dMesh* HeroConfig::getFeetMesh(int index) const
{
	r3d_assert(index>=0 && index < (int)m_FeetMeshes.size());
	if(m_FeetMeshes[index] == 0)
	{
		char tmpBuf[512];
		sprintf(tmpBuf, "%s_Shoes_%02d.sco", m_BaseModelName, index+1);
		if(!r3dFileExists(tmpBuf))
			sprintf(tmpBuf, "%s_Feet_%02d.sco", m_BaseModelName, index+1);

		m_FeetMeshes[index] = r3dGOBAddMesh(tmpBuf, true, false, true, true );
		if(m_FeetMeshes[index]==NULL)
		{
			r3dError("ART: failed to load mesh '%s'\n", tmpBuf);
		}
		r3d_assert(m_FeetMeshes[index]);
	}

	return m_FeetMeshes[index];
}

size_t HeroConfig::getNumHeads() const
{
	return m_HeadMeshes.size();
}

size_t HeroConfig::getNumLegs() const
{
	return m_LegsMeshes.size();
}

size_t HeroConfig::getNumBodys() const
{
	return m_BodyMeshes.size();
}

size_t HeroConfig::getNumHair() const
{
	return m_HairMeshes.size();
}

size_t HeroConfig::getNumFeet() const
{
	return m_FeetMeshes.size();
}

bool HeroConfig::loadBaseFromXml(pugi::xml_node& xmlHero)
{
	BaseItemConfig::loadBaseFromXml(xmlHero);

	m_BaseModelName = strdup(xmlHero.child("Model").attribute("file").value());

	m_Weight = R3D_MIN(xmlHero.attribute("Weight").as_float(), 100.0f); // max weight is 100
	m_damagePerc = xmlHero.child("HeroDesc").attribute("damagePerc").as_float()/100.0f; // from 30 -> 0.3
	m_damageMax = xmlHero.child("HeroDesc").attribute("damageMax").as_float();
	int numHeads = xmlHero.child("HeroDesc").attribute("maxHeads").as_int();
	int numBodys = xmlHero.child("HeroDesc").attribute("maxBodys").as_int();
	int numLegs = xmlHero.child("HeroDesc").attribute("maxLegs").as_int();
	int numHair = xmlHero.child("HeroDesc").attribute("maxHair").as_int();
	int numFeet = xmlHero.child("HeroDesc").attribute("maxFeet").as_int();
	m_ProtectionLevel = xmlHero.child("HeroDesc").attribute("ProtectionLevel").as_int();

	r3d_assert(m_HeadMeshes.size() == 0);
	r3d_assert(m_LegsMeshes.size() == 0);
	r3d_assert(m_BodyMeshes.size() == 0);
	r3d_assert(m_BodyMeshesFPS.size() == 0);
	r3d_assert(m_HairMeshes.size() == 0);
	r3d_assert(m_FeetMeshes.size() == 0);

	for(int i=0; i<numHeads; ++i)
		m_HeadMeshes.push_back(NULL);
	for(int i=0; i<numLegs; ++i)
		m_LegsMeshes.push_back(NULL);
	for(int i=0; i<numHair; ++i)
		m_HairMeshes.push_back(NULL);
	for(int i=0; i<numFeet; ++i)
		m_FeetMeshes.push_back(NULL);
	for(int i=0; i<numBodys; ++i)
	{
		m_BodyMeshes.push_back(NULL);
		m_BodyMeshesFPS.push_back(NULL);
	}

	return true;
}
