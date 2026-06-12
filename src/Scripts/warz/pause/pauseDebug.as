package warz.pause {
	public class pauseDebug {

		public	var	api:warz.pause.pause;

		static public function initDebug(api:warz.pause.pause) {
			
			api.addCategory (11, "storecat_Armor", 1, 2);
			api.addCategory (13, "storecat_Helmet", 1, 3);
			api.addCategory (20, "storecat_ASR", 0, 0);
			api.addCategory (21, "storecat_SNP", 0, 0);
			api.addCategory (22, "storecat_SHTG", 0, 0);
			api.addCategory (23, "storecat_MG", 0, 0);
			api.addCategory (25, "storecat_HG", 0, 1);
			api.addCategory (26, "storecat_SMG", 0, 0);
			api.addCategory (28, "storecat_UsableItem", 2, 4);
			api.addCategory (29, "storecat_MELEE", 0, 1);
			api.addCategory (30, "storecat_Food", 5, 4);
			api.addCategory (31, "storecat_Backpack", 2, 4);
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
			api.addItem(100003, 22, "HG Glock18", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100004, 25, "Hand Gun", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100005, 30, "Food", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100006, 32, "Medical", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100007, 13, "helment", "ddd", "weapons/HG_Glock18.png");
			api.addItem(100008, 11, "armour", "ddd", "weapons/HG_Glock18.png");

			api.addItem(100, 12, "backpack 8 slot", "ddd", "weapons/HG_Glock18.png", false, 16, 16);

			api.addBackpack (0, 100);
			api.addBackpack (1, 100);
			api.addBackpack (2, 101);

			api.addClientSurvivor("Sagor", 0, 666, Math.random() * 100000, 0,0, 0, 0, 0, 1, 100, 100, 100, 100, 32, 20, 0, 0, 0, "Good", "Hell", false);
			
			api.addBackpackItem (8, 10, 100000, 1, 0, 0, "Hahah is this a bad item");
			api.addBackpackItem (9, 10, 100006, 1, 0, 0);
			api.addBackpackItem (10, 10, 100007, 1, 0, 0);
			api.addBackpackItem (11, 10, 100008, 1, 0, 0);
			
			api.addBackpackItem (28, 10, 100005, 1, 0, 0);
			api.addBackpackItem (29, 10, 100004, 1, 0, 0);
			api.addBackpackItem (30, 10, 100004, 1, 0, 0);
			api.addBackpackItem (31, 10, 100004, 1, 0, 0);
			
			
			api.setMapIcon("weapons/ASR_FAL.png");
			api.showInventory (true);
			
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
			
			api.setOptions(1, 1, 0.95, 0.25, 0.1, 0.2, 0.3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.5, 1, 0, 0, 1, 2);
			
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
			
			api.showMap(true);
			api.showInventory(false);
			api.showMissions(false);
			api.showOptions(false);
		}
	}
}