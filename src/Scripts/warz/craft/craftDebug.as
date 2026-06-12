package warz.craft {
	public class craftDebug {

		public	var	api:warz.craft.craft;

		static public function initDebug(api:warz.craft.craft) {
			
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
			api.addCategory (25, "storecat_HG", 0, 1);
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
			api.addItem(100003, 20, "HG Glock18", "ddd", "weapons/HG_Glock18.png");
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
			api.addItem(200014, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
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
			api.addItem(200030, 30, "ASR AK74", "ddd", "weapons/ASR_AK74.png");
			
			api.addItem(30000, 50, "Recipe1", "desc1", "weapons/ASR_AK74.png");
			api.addRecipe(30000, "Recipe1", "desc1", "weapons/ASR_AK74.png");
			api.addRecipeComponent(30000, 200030, 1);
			api.addRecipeComponent(30000, 100008, 5);
			api.addRecipeComponent(30000, 100001, 10);

			api.addItem(30001, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30001, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipeComponent(30001, 100008, 1);
			api.addRecipeComponent(30001, 200030, 5);
			api.addRecipeComponent(30001, 200030, 10);

			api.addItem(30002, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30002, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addItem(30003, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30003, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addItem(30004, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30004, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addItem(30005, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30005, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addItem(30006, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30006, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addItem(30007, 50, "Recipe2", "desc2", "weapons/HG_Glock18.png");
			api.addRecipe(30007, "Recipe2", "desc2", "weapons/HG_Glock18.png");

			api.addItem(100, 12, "backpack 8 slot", "ddd", "weapons/HG_Glock18.png", false, 16, 16);
			api.addItem(101, 12, "backpack 8 slot", "ddd", "weapons/HG_Glock18.png", false, 16, 24);
			
			api.addBackpack (0, 100);
			api.addBackpack (1, 100);
			api.addBackpack (2, 101);

			api.addClientSurvivor("Sagor", 100, 666, Math.random() * 100000, 0, 1, 0, 0, 0, 1, 100, 100, 100, 100, 16, 32, 0, 0, 0, "Good", "Hell", true);
			
			api.addBackpackItem(0, 0, 200030, 1, 1, 1, "blalsa");
			api.addBackpackItem(1, 0, 100003, 1, 1, 1, "fgkjfd");
			api.addBackpackItem(2, 0, 100006, 5, 0, 0, "dfdf");
			api.addBackpackItem(3, 0, 100006, 5, 0, 0, "dfdf");
			api.addBackpackItem(6, 0, 100006, 5, 0, 0, "dfdf");
			
			api.addBackpackItem(10, 0, 100006, 5, 0, 0, "dfdf");
			api.addBackpackItem(11, 0, 100006, 5, 0, 0, "dfdf");
			
			api.showCraftScreen();
		}
	}
}
