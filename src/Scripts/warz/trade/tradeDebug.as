package warz.trade {
	public class tradeDebug {

		public	var	api:warz.trade.trade;

		static public function initDebug(api:warz.trade.trade) {
			
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

			api.addClientSurvivor("Sagor", 100, 666, Math.random() * 100000, 0, 1, 0, 0, 0, 1, 100, 100, 100, 100, 48, 32, 0, 0, 0, "Good", "Hell", true);
			
			for(var i=0; i<20; ++i)
				api.addBackpackItem(i, 100+i, 100000, 1, 0, 0);

			
			api.showScreen();
			
			api.showInfoMsg("dfhdkjf", true, "dfkhjdf");
		}
	}
}