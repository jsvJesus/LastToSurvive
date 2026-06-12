package warz.hud  {
	import flash.utils.Timer;

	public class HUDDebug {

		static public function initDebug(api:warz.hud.HUD) {
			api.showWeaponInfo (true);
			api.setWeaponInfo (Math.random() * 10, Math.random() * 10, "auto", 50);
			api.setSlot (0, "Test", 100, "items/PACK_NS.png");
			api.setSlot (1, "Test", 100, "items/PACK_NS.png");
			
			api.setChatTab (0, "General", true, true);
			api.setChatTab (1, "Friends", false, true);
			api.setChatTabActive (0);
			
			api.setActiveSlot (0);
			api.setActivatedSlot (1);
			
			api.setSlotCooldown(0, 105, 0);
			api.setSlotCooldown(1, 105, 0);
			api.setSlotCooldown(2, 30, 30);
			api.setActivatedSlot (0);
			
			api.startDebugTimer ();
			
			//api.showNoteWrite(true);
			api.showNoteRead(true, "djfhdkfjhd");
			
			var t = api.addCharTag("test test", true, "paragon");
			api.moveUserIcon(t, 500, 500, true);
			
			api.clearPlayersList();
			for(var i=0; i<200; ++i)
				api.addPlayerToList(i, i, "Test", "bad guy", false, false, false, false, false, true, true);
			api.showPlayersList(true);
			
			api.showSafelock(false, true);
			
			api.addPlayerToVoipList("test1");
			api.addPlayerToVoipList("test2");
			api.addPlayerToVoipList("test3");
			api.addPlayerToVoipList("test4");
			api.removePlayerFromVoipList("test3");
			
			api.showGraveNote(true, "bla bla\n\nbla bla", "pl1", "pl2");
			
			api.setThreatValue(100);
			
			api.addMissionInfo("Mission1");
			api.addMissionObjective(0, "Obj1", true, "56/90", true);
			api.addMissionObjective(0, "Obj2", false, "66/90", true);
			api.addMissionObjective(0, "Obj3", false, "76/90", true);
			api.addMissionObjective(0, "Obj4", false, "86/90", true);
			api.addMissionObjective(0, "Obj5", false, "96/90", false);
			api.setMissionObjectiveInArea(0, 2, true);
			api.addMissionInfo("Mission2");
			api.addMissionObjective(1, "Obj1", true, "56/90", false);
			api.addMissionObjective(1, "Obj2", false, "66/90", false);
			api.addMissionInfo("Mission3");
			api.addMissionObjective(2, "Obj3", true, "56/90", false);
			api.addMissionObjective(2, "Obj4", false, "66/90", false);
			
			api.removeMissionInfo(1);
			api.setMissionObjectiveNumbers(1, 1, "999");
			api.setMissionObjectiveCompleted(0, 4);
			
			
			api.showMissionInfo(true);
		}
	}
}