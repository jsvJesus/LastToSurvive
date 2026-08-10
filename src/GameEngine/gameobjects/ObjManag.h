#ifndef __PWAR_OBJMANAG_H
#define __PWAR_OBJMANAG_H

#include "GameObj.h"

#include <Platform/Synchronization.h>

#include <array>
#include <cstdint>
#include <unordered_map>

enum SnapType_t
{
    eSnapType_Pivot,
    eSnapType_Vertex,

    eSnapType_Count
};

struct SnapInfo_t
{
    SnapType_t eType;
    float fRadius;
};

struct SnapPointResult_t
{
    GameObject* pObj;
    r3dVector vPos;
};

struct draw_s
{
    GameObject* obj;
    float distSq;
    uint8_t shadow_slice; // 1,2,3 bit flag
};

#define OBJECTMANAGER_MAXOBJECTS 8192
#define OBJECTMANAGER_MAXSTATICOBJECTS 32768

#define OBJECTMANAGER_STATICBIT 0x80000000

#include "sceneBox.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Object Manager
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ObjectIterator
{
    GameObject* current;
    int staticIndex;
};

class ObjectManagerResourceHelper : public r3dIResource
{
public:
    ObjectManagerResourceHelper();
    virtual ~ObjectManagerResourceHelper();

    virtual void D3DCreateResource()
    {
    }

    virtual void D3DReleaseResource();
};

class ObjectManager
{
private:
    typedef void (*GameObjEvent_fn)(
        GameObject* pObj);

    GameObjEvent_fn pObjectAddEvent;
    GameObjEvent_fn pObjectDeleteEvent;

    int m_FrameId;
    SceneBox* m_pRootBox;
    ObjectManagerResourceHelper* m_ResourceHelper;

    engine::platform::Mutex m_Mutex;

public:
    bool GetSnapPoint(
        const r3dPoint2D& vCursor,
        const SnapInfo_t& tInfo,
        SnapPointResult_t& tRes);

    void DrawDebug(
        const r3dCamera& Cam);

    void AppendDebugBoxes();

    int GetFrameId();

    int MaxObjects = 0;

    GameObject* pFirstObject = nullptr;

    int NumObjects = 0;

    GameObject* pLastObject = nullptr;

    int bInited = 0;

    int CurObjID = 1;

    GameObject** pObjectArray = nullptr;

    int LastFreeObject = 0;

    int MaxStaticObjects = 0;

    int NumStaticObjects = 0;

    GameObject** pStaticObjectArray = nullptr;

    int LastStaticUpdateIdx;

    typedef std::unordered_map<
        std::uint32_t,
        GameObject*>
        NetMapType;

    NetMapType NetworkIDMap;

    // Used for particle shadow casting only.
    std::array<GameObject*, 512>
        TransparentShadowCasters{};

    int TransparentShadowCasterCount;

    r3dCamera PrepCam;
    r3dCamera PrepCamInterm;

    r3dPoint3D m_MinimapOrigin;
    r3dPoint3D m_MinimapSize;

#ifndef WO_SERVER
    class BulletShellMngr* m_BulletMngr;
#endif

    int JustLoaded;

public:
    ObjectManager();
    ~ObjectManager();

    int Init(
        int MaxObjects,
        int MaxStaticObjects);

    int Destroy();

    void OnResetDevice();

    int GetNumObjects() const
    {
        return NumObjects;
    }

    void SetAddObjectEvent(
        GameObjEvent_fn pEvent);

    void SetDeleteObjectEvent(
        GameObjEvent_fn pEvent);

    int AddObject(
        GameObject* obj);

    int DeleteObject(
        GameObject* obj,
        bool call_delete = true);

    void LinkObject(
        GameObject* obj);

    void UnlinkObject(
        GameObject* obj);

    int SetDrawingOrder(
        GameObject* obj,
        int order);

    GameObject* GetObject(
        gobjid_t ID);

    GameObject* GetObject(
        const char* name);

    GameObject* GetObjectByHash(
        uint32_t hash);

    GameObject* GetFirstObject();

    GameObject* GetNextObject(
        const GameObject* obj);

    ObjectIterator GetFirstOfAllObjects();

    ObjectIterator GetNextOfAllObjects(
        const ObjectIterator& it);

    GameObject* GetStaticObject(
        int idx);

    int GetStaticObjectCount() const;

    GameObject* GetNetworkObject(
        std::uint32_t netID);

    void GetObjectsInCube(
        const r3dBoundBox& box,
        GameObject**& result,
        int& objectsCount);

    void StartFrame();
    void Update();
    void EndFrame();

    void DumpObjects();

    void PrepareSlicedShadowsInterm(
        const r3dCamera& Cam,
        D3DXPLANE (&mainFrustumPlanes)[6]);

    void PrepareShadowsInterm(
        const r3dCamera& Cam);

    void PrepareTransparentShadowsInterm(
        const r3dCamera& Cam);

    void Prepare(
        const r3dCamera& Cam);

    // Force managed resources into video memory.
    void WarmUp();

    void IssueOcclusionQueries();

    void Draw(
        eRenderStageID DrawState);

    void DrawIntermediate(
        eRenderStageID DrawState);

    void ResetObjFlags();

    GameObject* CastRay(
        const r3dPoint3D& pos,
        const r3dPoint3D& vRay,
        float RayLen,
        CollisionInfo* cInfo,
        int bboxonly = false);

    GameObject* CastMeshRay(
        const r3dPoint3D& pos,
        const r3dPoint3D& vRay,
        float RayLen,
        CollisionInfo* cInfo);

    GameObject* CastQuickRay(
        const r3dPoint3D& pos,
        const r3dPoint3D& vRay,
        float RayLen,
        CollisionInfo* cInfo);

    GameObject* CastBBoxRay(
        const r3dPoint3D& pos,
        const r3dPoint3D& vRay,
        float RayLen,
        CollisionInfo* cInfo);

    int SendEvent_to_All(
        int event,
        void* data);

    int SendEvent_to_ObjClass(
        const char* name,
        int event,
        void* data);

    int SendEvent_to_ObjName(
        const char* name,
        int event,
        void* data);

    void RecalcIntermObjectMatrices();
    void RecalcObjectMatrices();

    SceneBox* GetRoot() const;

    void UpdateTransparentShadowCaster(
        GameObject* obj);

    void OnGameEnded();

private:
    void DoPreparedDraw(
        const r3dCamera& Cam,
        eRenderStageID DrawState);

    void AddToTransparentShadowCasters(
        GameObject* obj);

    void RemoveFromTransparentShadowCasters(
        GameObject* obj);
};

// Game world creation / destruction / access.
extern void GameWorld_Create();
extern void GameWorld_Destroy();
extern ObjectManager& GameWorld();

// Object creation flags.
#define OBJ_CREATE_LOCAL       (1 << 0)
#define OBJ_CREATE_DYNAMIC     (1 << 1)
#define OBJ_CREATE_SKIP_LOAD   (1 << 5)
#define OBJ_CREATE_SKIP_POS    (1 << 6)

extern GameObject* srv_CreateGameObject(
    int class_type,
    const char* load_name,
    const r3dPoint3D& Pos,
    long flags = 0,
    void* data = NULL);

extern GameObject* srv_CreateGameObject(
    const char* class_name,
    const char* load_name,
    const r3dPoint3D& Pos,
    long flags = 0,
    void* data = NULL);

bool DoesShadowCullNeedRecalc();

void PrecalculateWorldMatrices(
    void* Data,
    size_t ItemStart,
    size_t ItemCount);

#endif // __PWAR_OBJMANAG_H
