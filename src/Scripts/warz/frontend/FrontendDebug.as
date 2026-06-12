package warz.frontend  {
	public class FrontendDebug {
		static public function initDebug(api:warz.frontend.Frontend) {
			api.setLanguage("english");
			
			
			api.addTabType(0,  "weapon", 	false, true);
			api.addTabType(1,  "ammo", 		true, true);
			api.addTabType(2,  "explosives",false, true);
			api.addTabType(3,  "gear",		true, true);
			api.addTabType(4,  "food", 		true, true);
			api.addTabType(5,  "survival", 	true, true);
			api.addTabType(6,  "equipment", true, true);
			api.addTabType(7,  "account", 	true, false);
			
			api.addCategory (11, "storecat_Armor", 3, 2);
			api.addCategory (13, "storecat_Helmet", 3, 3);
			api.addCategory (20, "storecat_ASR", 0, 0);
			api.addCategory (21, "storecat_SNP", 0, 0);
			api.addCategory (22, "storecat_SHTG", 0, 0);
			api.addCategory (23, "storecat_MG", 0, 0);
			api.addCategory (25, "storecat_HG", 0, 1, 0);
			api.addCategory (26, "storecat_SMG", 0, 0);
			api.addCategory (28, "storecat_UsableItem", 3, 4);
			api.addCategory (29, "storecat_MELEE", 0, 1);
			api.addCategory (30, "storecat_Food", 4, 4);
			api.addCategory (31, "storecat_Backpack", 6, 4);
			api.addCategory (32, "storecat_Medical", 4, 4);
			api.addCategory (33, "storecat_Water", 5, 4);			
			api.addCategory (16, "storecat_HeroPackage", -1, -1);
			api.addCategory (12, "storecat_Backpack", 2, -1);

			api.addItem(1, 16, "Hero", "ddd", "weapons/HG_Glock18.png");
			api.addItem(2, 16, "Hero", "ddd", "weapons/HG_Glock18.png");
			api.addItem(3, 16, "Hero", "ddd", "weapons/HG_Glock18.png");
			api.addItem(4, 16, "Hero", "ddd", "weapons/HG_Glock18.png");

			api.addItem(100000, 20, "ASR FAL", "ddd", "weapons/ASR_FAL.png");
			api.addItem(100001, 20, "ASR AKM", "ddd", "weapons/ASR_AKM.png");
			api.addItem(100002, 20, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(100003, 25, "HG Glock18", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100004, 25, "Hand Gun", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100005, 30, "Food", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100006, 32, "Medical", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100007, 20, "helment", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100008, 11, "armour", "ddd", "weapons/HG_Glock18.png");

			api.addItem(200002, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200003, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200004, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200005, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200006, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200007, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200008, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200009, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200010, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200011, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200012, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200013, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			/*api.addItem(200014, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200015, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200016, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200017, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200018, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200019, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200020, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200021, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200022, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200023, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200024, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200025, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200026, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200027, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200028, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200029, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			api.addItem(200030, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");*/

			api.addItem(100, 12, "backpack 8 slot", "ddd", "weapons/HG_Glock18.png", false, 16, 16);
			api.addItem(101, 12, "backpack 8 slot", "ddd", "weapons/HG_Glock18.png", false, 16, 24);
			
			api.addHero(1, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(2, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(3, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			/*api.addHero(4, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(5, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(6, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(7, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(8, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(9, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png", 2, 2, 2);
			api.addHero(10, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(11, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(12, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(13, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(14, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(15, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(16, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			api.addHero(17, 0, "Test", "getst", "items/createsurv_charslot_pictest.png", "items/createsurv_charslot_pictest2.png", "items/char3.png",2, 2, 2);
			*/
			api.addBackpack (0, 100);
			api.addBackpack (1, 100);
			api.addBackpack (2, 101);

			api.setCells(int (Math.random() * 1000));
			api.setDollars(int (Math.random() * 1000));
			api.setGC(int (Math.random() * 1000));
			
			api.addClientSurvivor("Sagor", 100, 666, Math.random() * 100000, 0, 1, 0, 0, 0, 1, 100, 100, 100, 100, 36, 32, 0, 0, 0, "Good", "Hell", true, 100000);
			api.addClientSurvivor("Matt", 50, 777, Math.random() * 100000, 0, 1, 0, 0, 0, 0, 100, 100, 100, 100, 16, 16, 0, 0, 0, "Good", "Hell", false, 50);
			api.addClientSurvivor("phil", 50, 777, Math.random() * 100000, 0, 1, 0, 0, 0, 0, 100, 100, 100, 100, 16, 24, 0, 0, 0, "Good", "Hell", false, 500000);
			
			api.addInventoryItem (1, 100000, 1, 0, 0, false, "This is a test string");
			api.addInventoryItem (2, 100001, 1, 0, 0, false);
			api.addInventoryItem (3, 100003, 3, 0, 0, false);
			api.addInventoryItem (4, 100004, 3, 0, 0, false);
			api.addInventoryItem (5, 100005, 3, 0, 0, false);
			api.addInventoryItem (6, 100006, 3, 0, 0, false);
			api.addInventoryItem (7, 100007, 3, 0, 0, false);
			api.addInventoryItem (8, 1, 3, 0, 0, false);
			api.addInventoryItem (9, 100007, 3, 0, 0, false);
			api.addInventoryItem (10, 100007, 3, 0, 0, false);
			api.addInventoryItem (11, 100007, 3, 0, 0, false);
			api.addInventoryItem (12, 100007, 3, 0, 0, false);
			api.addInventoryItem (13, 100007, 3, 0, 0, false);
			api.addInventoryItem (14, 100007, 3, 0, 0, false);
			api.addInventoryItem (15, 100003, 3, 0, 0, false);
			api.addInventoryItem (16, 100007, 3, 0, 0, false);
			api.addInventoryItem (17, 100007, 3, 0, 0, false);
			api.addInventoryItem (18, 100007, 3, 0, 0, false);
			api.addInventoryItem (19, 100003, 3, 0, 0, false);
			api.addInventoryItem (20, 100007, 3, 0, 0, false);
			api.addInventoryItem (21, 100007, 3, 0, 0, false);
			api.addInventoryItem (22, 100007, 3, 0, 0, false);
			api.addInventoryItem (23, 100000, 3, 0, 0, false);
			api.addInventoryItem (24, 100007, 3, 0, 0, false);
			api.addInventoryItem (25, 100007, 3, 0, 0, false);
			api.addInventoryItem (26, 100000, 3, 0, 0, false);
			api.addInventoryItem (27, 100003, 3, 0, 0, false);
			api.addInventoryItem (28, 100007, 3, 0, 0, false);
			api.addInventoryItem (29, 100000, 3, 0, 0, false);
			api.addInventoryItem (30, 100000, 3, 0, 0, false);
			api.addInventoryItem (31, 100000, 3, 0, 0, false);
			
			api.addScreenResolution("100x100");
			api.addScreenResolution("200x100");
			api.addScreenResolution("300x100");
			api.addScreenResolution("400x100");
			api.addScreenResolution("500x100");
			api.addScreenResolution("600x100");
			api.addScreenResolution("600x100");
			api.addScreenResolution("600x100");
			api.addScreenResolution("600x100");
			api.addScreenResolution("600x100");
			api.addScreenResolution("600x100");
			
			api.setBrowseGamesOptions(true, false, false, false, true, true, false, true, true, true, false, true, false, false);
			
			api.setOptions(1, 1, 0.95, 0.25, 0.1, 0.2, 0.3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.5, 1, 0, 0, 1, 2);
			api.setVoipOptions(1, 1, 0, 1, 0.8);
			
			api.addKeyboardMapping("key1", "LEFT MOUSE BUTTON");
			api.addKeyboardMapping("key2", "RIGHT MOUSE BUTTON");
			api.addKeyboardMapping("key3", "fff");
			api.addKeyboardMapping("key4", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("keyx", "fff");
			api.addKeyboardMapping("last", "fff");
			
			api.addClanIcon("clan/frontend_clan_icons_lawman.png");
			api.addClanIcon("clan/frontend_clan_icons_survivor.png");
			api.addClanIcon("clan/frontend_clan_icons_bandit.png");
			
			api.addClanSlotBuyInfo(0, 100, 10);
			api.addClanSlotBuyInfo(1, 1000, 20);
			api.addClanSlotBuyInfo(2, 10000, 30);
			api.addClanSlotBuyInfo(3, 20000, 50);
			api.addClanSlotBuyInfo(4, 30000, 100);
			api.addClanSlotBuyInfo(5, 40000, 1000);

			api.showSurvivorsScreen();
//			api.showOptionsControls ();

//			api.showInfoMsg ("Test", true);
//			api.showLoginMsg ("Loading....");

			api.updateDeadTimer (Math.random () * 1000, Math.random () * 100, true);
			
			api.addRentServer_MapInfo(0, "Colorado");
			api.addRentServer_MapInfo(1, "Cliffside");
			api.addRentServer_MapInfo(2, "Cliffside2");
			api.addRentServer_MapInfo(3, "Cliffside3");
			api.addRentServer_MapInfo(4, "Cliffside4");
			api.addRentServer_MapInfo(5, "Cliffside5_LAST");
			
			api.addRentServer_StrongholdInfo(0, "S1");
			api.addRentServer_StrongholdInfo(1, "S2");
			api.addRentServer_StrongholdInfo(2, "S3");
			
			api.addRentServer_RegionInfo(0, "US");
			api.addRentServer_RegionInfo(1, "EU");
			api.addRentServer_RegionInfo(2, "SA");
			
			api.addRentServer_SlotsInfo(32, "32", false);
			api.addRentServer_SlotsInfo(64, "64", true);
			api.addRentServer_SlotsInfo(128, "128", true);
			api.addRentServer_SlotsInfo(256, "256", true);
			api.addRentServer_SlotsInfo(1024, "1024", false);

			api.addRentServer_SlotsInfoStronghold(32, "32", false);
			api.addRentServer_SlotsInfoStronghold(64, "64", true);
			api.addRentServer_SlotsInfoStronghold(128, "128", true);
			api.addRentServer_SlotsInfoStronghold(256, "256", true);
			api.addRentServer_SlotsInfoStronghold(1024, "1024", false);

			api.addRentServer_RentInfo(0, 1, "WEEK");
			api.addRentServer_RentInfo(1, 1, "MONTH");
			api.addRentServer_RentInfo(2, 2, "MONTH");
			api.addRentServer_RentInfo(3, 3, "MONTH");
			api.addRentServer_RentInfo(4, 6, "MONTH");
			
			api.addRentServer_PVEInfo(0, "NO");
			api.addRentServer_PVEInfo(1, "YES");
			
			for(var i=0; i<34; ++i)
				api.addSkillInfo(i, "skill"+i, "desc"+i, "skill/skill.png", "skill/skillBW.png", 100+i*10);			
				
			api.setSkillLearnedSurvivor("Sagor", 0);
			api.setSkillLearnedSurvivor("Sagor", 1);
			api.setSkillLearnedSurvivor("Sagor", 2);
			
			//api.Main.DonateGCSrvPopUp.showPopUp(0);
		}
	}	
}

