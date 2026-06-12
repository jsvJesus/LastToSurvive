package warz.frontend  {
	import flash.events.*;
	import flash.display.MovieClip;
	import flash.net.URLRequest;
	import flash.display.Loader;
	import warz.utils.Layout;
	import flash.display.Bitmap;
	import caurina.transitions.Tweener;
	import warz.events.FrontEndEvents;
	import flash.text.TextFormat;
	import flash.text.TextField;
	import scaleform.gfx.Extensions;
	import warz.dataObjects.*;
	
	public class Frontend  {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;
		
		static public var api:Frontend = null;
		
		public var m_Language:String = "english";
		public var tweenDelay:Number = 0.2;
	
		public var m_DisableLanguageSelection:Boolean = false;

		public 	var Survivors:Array = null;
		public	var	CategoryDB:Array = null;
		public	var	InventoryDB:Array = null;
		public	var	PackageDB:Array;
		public	var	ItemDB:Array = null;
		public	var	StoreDB:Array = null;
		public	var	TabDB:Array = null;
		public	var	SlotDB:Array = null;
		public	var	HeroDB:Array = null;
		public	var	BackpackDB:Array = null;
		
		public  var myClanInfo:ClanInfo = new ClanInfo();
		public  var myClanIcons:Array = new Array();
		public  var myClanSlotsBuyInfo:Array = new Array();
		
		public	var	keyboardMapping:Array = null;
		
		public var Opt_ScreenResolutions:Array = null;
		public var Opt_ScreenResIndex :Number;
		public var Opt_OverallQ :Number;
		public var Opt_Brightness :Number;
		public var Opt_Contrast :Number;
		public var Opt_SoundV :Number;
		public var Opt_MusicV :Number;
		//public var Opt_CommV :Number;
		public var Opt_hintS :Number;
		public var Opt_RedBlood :Number;
		public var Opt_AA :Number;
		public var Opt_VSync :Number;
		public var Opt_Fullscreen :Number;
		public var Opt_SSAO :Number;
		public var Opt_TerrainD :Number;
		public var Opt_TerrainT :Number;
		public var Opt_WaterQ :Number;
		public var Opt_ShadowQ :Number;
		public var Opt_LightQ :Number;
		public var Opt_EffectsQ :Number;
		public var Opt_MeshD :Number;
		public var Opt_Aniso :Number;
		public var Opt_PostProc :Number;
		public var Opt_MeshT :Number;
	
		public var Opt_vertLook :Number;
		public var Opt_leftSideCam :Number;
		public var Opt_mouseWheel :Number;
		public var Opt_mouseSens :Number;
		public var Opt_mouseAccel :Number;
		public var Opt_toggleAim :Number;
		public var Opt_toggleCrouch :Number;		
		
		public var Opt_EnableVoiceChat : Number;
		public var Opt_ShowChatBubble : Number;
		public var Opt_InputDevice : Number;
		public var Opt_OutputDevice : Number;
		public var Opt_VoiceChatVolume : Number;
		
		public var BrowseGames_Region_US:Boolean;
		public var BrowseGames_Region_EU:Boolean;
		public var BrowseGames_Region_RU:Boolean;
		public var BrowseGames_Region_SA:Boolean; // south america
		
		public var BrowseGames_Filter_Gameworld:Boolean;
		public var BrowseGames_Filter_Stronghold:Boolean;
		public var BrowseGames_Filter_HideEmpty:Boolean;
		public var BrowseGames_Filter_HideFull:Boolean;
		public var BrowseGames_Filter_PrivateServers:Boolean;
		
		public var BrowseGames_Options_Tracers:Boolean;
		public var BrowseGames_Options_Nameplates:Boolean;
		public var BrowseGames_Options_Crosshair:Boolean;
		public var BrowseGames_Options_Password:Boolean;
		public var BrowseGames_Options_Enable:Boolean;
		
		public var RentServer_Maps:Array = new Array();
		public var RentServer_MapsStronghold:Array = new Array();
		public var RentServer_Region:Array = new Array();
		public var RentServer_Slots:Array = new Array();
		public var RentServer_SlotsStronghold:Array = new Array();
		public var RentServer_Rental:Array = new Array();
		public var RentServer_PVE:Array = new Array();
		
		public var SkillData:Array = new Array();
		
		public var EarlyRevival_Price : uint;
		public var PremiumAccount_Price : uint;
		public var ChangeName_Price : uint;
		public var isTrialAccount:Boolean = false;
		
		public	var	language:int = 0;		
		private	var	curSurvivor:int = -1;

// basic stats
		public	var	money:Object;

		public function Frontend(main:MovieClip) 
		{
			api = this;
			Main = main;
			
			scaleform.gfx.Extensions.enabled = true;
			
			Tweener.init();
			
			money = {dollars:0, gc:0, cells:0};
			
			Survivors = new Array();		
			CategoryDB = new Array ();
			TabDB = new Array ();
			BackpackDB = new Array ();
			InventoryDB = new Array ();
			ItemDB = new Array ();
			StoreDB = new Array ();
			PackageDB = new Array ();
			Opt_ScreenResolutions = new Array ();
			keyboardMapping = new Array();
			HeroDB = new Array ();
			SlotDB = new Array ([], [], [], [], []);
		}
		
		public function upgradeTrialAccountCallback(ok:Boolean, inputText:String)
		{
			FrontEndEvents.eventTrialUpgradeAccount(inputText);
		}
		
		public function showTrialUpgradeWindow()
		{
			Main.MsgBox.showInfoInputMsg("$FR_EnterCodeToUpgradeTrialAccount", "$FR_UpgradeTrialAccount", upgradeTrialAccountCallback);
		}
		
		public function addSkillInfo(id:uint, name:String, desc:String, icon:String, iconBW:String, cost:uint)
		{
			SkillData.push({id:id, name:name, desc:desc, icon:icon, iconBW:iconBW, cost:cost});
			if(SkillData.length > 34)
			{
				trace("[ERROR]: too many skills added");
			}
		}
		
		public function addRentServer_MapInfo(id:uint, name:String)
		{
			RentServer_Maps.push({id:id, displayValue:name});
		}
		public function addRentServer_StrongholdInfo(id:uint, name:String)
		{
			RentServer_MapsStronghold.push({id:id, displayValue:name});
		}
		public function addRentServer_RegionInfo(id:uint, name:String)
		{
			RentServer_Region.push({id:id, displayValue:name});
		}
		public function addRentServer_SlotsInfo(id:uint, name:String, enabled:Boolean)
		{
			RentServer_Slots.push({id:id, displayValue:name, enabled:enabled});
		}
		public function addRentServer_SlotsInfoStronghold(id:uint, name:String, enabled:Boolean)
		{
			RentServer_SlotsStronghold.push({id:id, displayValue:name, enabled:enabled});
		}
		public function addRentServer_RentInfo(id:uint, num:uint, name:String)
		{
			RentServer_Rental.push({id:id, num:num, displayValue:name});
		}
		public function addRentServer_PVEInfo(id:uint, name:String)
		{
			RentServer_PVE.push({id:id, displayValue:name});
		}

		public function addClanSlotBuyInfo(buyIdx:uint, price:uint, numSlots:uint)
		{
			myClanSlotsBuyInfo.push({buyIdx:buyIdx, price:price, numSlots:numSlots});
		}

		public function showClanInvite(inviteID:uint, clanName:String, numMembers:uint, desc:String, iconID:uint)
		{
			Main.ClanInvitePopup.showInvite(inviteID, clanName, numMembers, desc, myClanIcons[iconID]);
		}
		
		public function addClanIcon(icon:String)
		{
			myClanIcons.push(icon);
		}		
		
		public function setClanInfo(clanID:uint, isAdmin:Boolean, name:String, availableSlots:uint, clanReserve:uint, logoID:uint)
		{
			myClanInfo.clanMembers = new Array();
			myClanInfo.myClanID = clanID;
			myClanInfo.isAdmin = isAdmin;
			myClanInfo.myName = name;
			myClanInfo.availableSlots = availableSlots;
			myClanInfo.clanReserve = clanReserve;
			myClanInfo.logoID = logoID;
		}
		public function addClanMemberInfo(charID:uint, name:String, exp:uint, time:String, rep:String, kzombie:uint, ksurvivor:uint, kbandits:uint, donatedgc:uint, rank:uint)
		{
			myClanInfo.clanMembers.push({charID:charID, id:myClanInfo.clanMembers.length+1, name:name, exp:exp, time:time, rep:rep, kzombie:kzombie, ksurvivor:ksurvivor, kbandits:kbandits, donatedgc:donatedgc, movie:null, rank:rank});
		}
		
		public function clearInventory()
		{
			InventoryDB = new Array();
		}
		
		public function clearBackpack()
		{
			var survivor:Survivor = Survivors[SelectedChar];
			
			survivor.clearBackpack ();
		}
		
		public function setSkillLearnedSurvivor(Name:String, skillID:int)
		{
			for (var s:int = 0; s < Survivors.length; s++)
			{
				var	survivor:Survivor = Survivors[s];
				
				if (survivor.Name == Name)
				{
					survivor.Skills[skillID] = 1;
					break;
				}
			}
		}
		
		public function changeSurvivorName(prevName:String, newName:String)
		{
			for (var s:int = 0; s < Survivors.length; s++)
			{
				var	survivor:Survivor = Survivors[s];
				
				if (survivor.Name == prevName)
				{
					survivor.Name = newName;
					Main.SurvivorsAnim.updateSurvivors();
					break;
				}
			}			
		}
		
		public	function updateClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize:int, skillXPPool:int)
		{
			for (var s:int = 0; s < Survivors.length; s++)
			{
				var	survivor:Survivor = Survivors[s];
				
				if (survivor.Name == Name)
				{
					survivor.health = health;
					survivor.xp = xp;
					survivor.timeplayed = timeplayed;
					survivor.hunger = hunger;
					survivor.alive = alive;
					survivor.thirst = thirst;
					survivor.toxic = toxic;
					survivor.SkillXPPool = skillXPPool;
					
					if (survivor.backpackItem.itemID != backpackID)
					{
						var	backpack:Item = getItemByID(backpackID);
						
						survivor.setBackpack (backpack, backpackSize);
					}
					break;
				}
			}
		}
		
		public function updateClientSurvivorWeight(Name:String, weight:Number)
		{
			for (var s:int = 0; s < Survivors.length; s++)
			{
				var	survivor:Survivor = Survivors[s];
				
				if (survivor.Name == Name)
				{
					survivor.weight = weight;
				}
			}
		}
		
		public	function addClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											hardcore:int, heroID:int, headID:int, bodyID:int, legsID:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize:int, weight:Number,
											zombiesKilled:int, banditsKilled:int, civiliansKilled:int,
											alignment:String, lastMap:String, globalInventory:Boolean,
											skillXPPool:int)
		{
			if(alive == 0)
				health = 0;
			var	survivor:Survivor = new Survivor (Name, health, xp, timeplayed, hardcore, heroID, headID, bodyID, legsID, 
												  alive, hunger, thirst, toxic, weight, 
												  zombiesKilled, banditsKilled, civiliansKilled, alignment, lastMap, globalInventory, skillXPPool);
												  
			var	backpack:Item = getItemByID(backpackID);
			
			if (backpack)
			{
				survivor.setBackpack (backpack, backpackSize);
			}
			
			Survivors.push(survivor);
			if(Survivors.length > 5)
				trace("[ERROR]: More than 5 survivors added");
		}
		
		public function getInventoryItemByID(itemID:uint):InventoryItem
		{
			var item:InventoryItem = null;
			
			for (var a = 0; a < InventoryDB.length; a++)
			{
				var	i:InventoryItem = InventoryDB[a];
				if (i.itemID == itemID)
				{
					item = InventoryDB[a];
				}
			}
			
			return item;
		}
		
		public function getInventoryItemByInventoryID(inventoryID:uint):InventoryItem
		{
			var item:InventoryItem = null;
			
			for (var a = 0; a < InventoryDB.length; a++)
			{
				var	i:InventoryItem = InventoryDB[a];
				
				if (i.inventoryID == inventoryID)
				{
					item = InventoryDB[a];
				}
			}
			
			return item;
		}
		
		public function getInventorySlot(slotID:Number):InventoryItem
		{
			if (slotID < InventoryDB.length)
				return InventoryDB[slotID];
				
			return null;
		}
		
		public function addInventoryItem(inventoryID:uint, itemID:uint, quantity:Number, var1:int, var2:int, isConsumable:Boolean, description:String = "")
		{
			var	it:InventoryItem = getInventoryItemByInventoryID(inventoryID);
			if(it != null)
			{
				it.quantity = quantity;
				it.var1 = var1;
				it.Description = description;
			}
			else
			{
				InventoryDB.push(new InventoryItem (inventoryID, itemID, quantity, var1, var2, isConsumable, description));
			}
		}
		
		public function removeInventoryItem(itemID:uint):void
		{
			for (var i:int = 0; i < InventoryDB.length; i++)
			{
				var	inventoryItem:InventoryItem = InventoryDB[i];
				
				if (inventoryItem.itemID == itemID)
				{
					InventoryDB.splice(i, 1);
					return;
				}
			}
		}
		
		public function addPackageInfo(itemID:uint, gp:String, gd:String, sp:String)
		{
			PackageDB.push({itemID:itemID, gp:gp, gd:gd, sp:sp, items:new Array()});
		}
		
		public function addItemToPackage(itemID:uint, item1:Number, item1Exp:Number, item2Exp:Number)
		{
			var	packageItem = getPackageItemByID (itemID);
			
			if (packageItem)
			{
				packageItem["items"].push({id:item1, exp:item1Exp, exp2:item2Exp});
			}
		}
		
		public function showSurvivorsScreen()
		{
			Main.MainMenu.SurvivorsBtn.dispatchEvent(new Event(MouseEvent.CLICK));
		}

		public function showMarketPlace()
		{
			Main.MainMenu.MarketplaceBtn.dispatchEvent(new Event(MouseEvent.CLICK));
		}

		public function showOptions()
		{
			Main.MainMenu.OptionsBtn.dispatchEvent(new Event(MouseEvent.CLICK));
		}

		public function showInventory()
		{
			Main.showScreen("Inventory");
		}
		public function showSkillTree()
		{
			Main.showScreen("SkillTree");
		}
		
		public	function showCreateSurvivor ()
		{
			Main.showScreen("CreateSurvivor");
		}

		public	function showOptionsGeneral ()
		{
			Main.showScreen("OptionsGeneral");
		}

		public	function showOptionsLanguage ()
		{
			Main.showScreen("OptionsLang");
		}

		public	function showOptionsVoip ()
		{
			Main.showScreen("OptionsVoip");
		}

		public	function showOptionsControls ()
		{
			Main.showScreen("OptionsControl");
		}

		public function addTabType(tabID:Number, name:String, visibleInStore:Boolean, visibleInInventory:Boolean):void
		{
			TabDB.push({tabID:tabID, name:name, inStore:visibleInStore, inInventory:visibleInInventory, categories:new Array ()});
		}
		
		public	function getInventoryTabs ():Array
		{
			var	tabs:Array = new Array ();
			
			for (var a:int = 0; a < TabDB.length; a++)
			{
				if (TabDB[a].inInventory)
				{
					tabs.push (TabDB[a]);
				}				
			}
		
			return tabs;
		}
		
		public	function getStoreTabs ():Array
		{
			var	tabs:Array = new Array ();
			
			for (var a:int = 0; a < TabDB.length; a++)
			{
				if (TabDB[a].inStore)
				{
					tabs.push (TabDB[a]);
				}				
			}
		
			return tabs;
		}
		
		/* 
		 * addCategory
		 * 
		 * catID
		 * name
		 * tabID - Relates to Tab ID uploaded in addTabType
		 * slotID - Relates to Weapon Slot position in backpack
		 */
		public function addCategory(catID:Number, name:String, tabID:int, slotID:int, slotID2:int=-1):void
		{
			var	cat:Object = {catID:catID, name:name}; 
			
			for (var a:int = 0; a < TabDB.length; a++)
			{
				var	tab = TabDB[a];
				
				if (tab["tabID"] == tabID)
				{
					tab["categories"].push (cat);
				}
			}
			
			if (slotID != -1)
				SlotDB[slotID].push (cat);			
			if (slotID2 != -1)
				SlotDB[slotID].push (cat);			
				
			CategoryDB.push (cat);
		}
		
		public	function findItemSlot (catID:Number):Array
		{
			var res:Array = new Array();
			for (var a:int = 0; a < 5; a++)
			{
				var	items:Array = SlotDB[a];
				
				for (var s:int = 0; s < items.length; s++)
				{
					var	item = items[s];
					
					if (item["catID"] == catID)
					{
						res.push(a);
					}
				}
			}
			if(res.length==0)
				res.push(-1);
			return res;
		}
		
		public function addItem(itemID:uint, catID:Number, name:String, desc:String, icon:String, 
								isStackable:Boolean = false, weight:Number = 0, maxSlots:int = 1, maxWeight:Number = 0):void
		{
			name = name.toUpperCase();
			ItemDB.push(new Item (itemID, catID, name, desc, icon, new Array(), isStackable, weight, maxSlots, maxWeight));
			
			if(isDebug)
			{
				addItemStat(itemID, "damage", 20+Math.random() * 60, 20, 80, true, "damage", false);
				addItemStat(itemID, "spread", 20+Math.random() * 60, 20, 80, true, "spread", true);
				addItemStat(itemID, "recoil", 20+Math.random() * 60, 20, 80, true, "recoil", true);
				addItemStat(itemID, "firerate", 20+Math.random() * 60, 20, 80, true, "firerate", false);
				addItemStat(itemID, "range", 20+Math.random() * 60, 20, 80, false, "", false);
				addItemStat(itemID, "capacity", 20+Math.random() * 60, 20, 80, false, "clipsize", false);
				
				if (Math.random() < 0.5)
					addStoreItem(itemID, 0, int (Math.random() * 50), 1, true);
				else
					addStoreItem(itemID, int (Math.random() * 5), 0, 1);
			}
		}
		
		public function getItemByID(itemID:uint):Item
		{
			var item:Item;
			
			for (var a = 0; a < ItemDB.length; a++)
			{
				var	i:Item = ItemDB[a];
				if (i.itemID == itemID)
				{
					item = ItemDB[a];
				}
			}
			
			if(item == null)
			{
				item = new Item (itemID, 0, "$FR_EmptySlot", "", "", new Array(), false, 0, 0, 0);
				return item;
			}
			
			return item;
		}
	
		public function getPackageItemByID(itemID:uint):Object
		{
			var item:Object = null;
			
			for (var a = 0; a < PackageDB.length; a++)
			{
				if (PackageDB[a]["itemID"] == itemID)
				{
					item = PackageDB[a];
				}
			}
			
			return item;
		}
	
		public function addItemStat(itemID:uint, statName:String, statValue:Number, statMin:Number, statMax:Number, progressBar:Boolean, upgradeType:String, isReversed:Boolean):void
		{
			var	item = getItemByID (itemID);
			
			if(item != null)
			{
				item["stats"].push({name:statName, value:statValue, statMin:statMin, statMax:statMax, progressBar:progressBar, upgradeType:upgradeType, isReversed:isReversed});
				
				if(item["stats"].length > 6)
					trace("[ERROR]: item has more than 6 stats! itemID="+itemID+", statName="+statName);
			}
		}
		
		public function addStoreItem(itemID:uint, price:Number, priceGD:Number, quantity:Number = 1, newItem:Boolean = false)
		{
			StoreDB.push(new StoreItem (itemID, price, priceGD, quantity, newItem));
		}	
		
		public function getStoreItemByID (itemID:uint):StoreItem
		{
			for (var a:int = 0; a < StoreDB.length; a++)
			{
				var storeItem:StoreItem = StoreDB[a];
				
				if (storeItem.itemID == itemID)
					return storeItem;
			}
			
			return null;
		}
		
		public function deleteCharSuccessful()
		{
			Survivors.splice(SelectedChar, 1);
			SelectedChar = 0;
			Main.updateSurvivors ();
		}
		
		public function reviveCharSuccessful()
		{
			var	s:Survivor = Survivors[SelectedChar];
			s.health = 100;
			s.alive = 1;
			s.globalInventory = true;
			Main.updateSurvivors ();
		}
		
		public function createCharSuccessful (slot:int)
		{
			SelectedChar = slot;
			
			Main.showScreen("Survivors");
			Main.updateSurvivors ();
		}
		
		public function createCharFailed ()
		{
			Main.updateSurvivors ();
		}

		public function showInfoOkCancelMsg(msg:String, title:String = "", callback:* = null)
		{
			Main.showInfoOkCancelMsg (msg, title, callback);
		}
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			Main.showInfoMsg (msg, OKButton, title);
		}
		
		private function showTrialAccountCallback(okPressed:Boolean)
		{
			if(okPressed)
				FrontEndEvents.eventTrialRequestUpgrade();
		}
		public function showTrialAccountLimit(msg:String, title:String)
		{
			Main.showInfoOkCancelMsg (msg, title, showTrialAccountCallback);
		}
		
		public function showMainMenu()
		{
			Main.MainMenu.visible = true;
		}
		
		public function hideMainMenu()
		{
			Main.MainMenu.visible = false;
		}
		
		public function showLoginMsg(title:String)
		{
			Main.LogIn.visible = true;
			hideMainMenu ();
			
			Main.LogIn.Block.Name.text = title;
			updateLoginMsg (0.0);
		}
		
		public function updateLoginMsg(percent:Number)
		{
			var barW:Number = 549;//Main.LogIn.Block.Bar.width;
			//percent = Math.min (percent, 1);
			//percent = Math.max (percent, 0);			
			Main.LogIn.Block.Bar.Scale.width = percent * barW;
		}
		
		public function hideLoginMsg()
		{
			Main.LogIn.visible = false;
			
			showMainMenu ();
		}
		
		public function updateInfoMsg(msg:String)
		{
			Main.PopUpAnim.PopUp.Text.text = msg;
		}
		
		public function hideInfoMsg()
		{
			Main.MsgBox.visible = false; 
		}
		
		public	function setDollars (dollars:int)
		{
			money.dollars = dollars;
			if(Main.Marketplace.isActiveScreen)
				Main.Marketplace.setNewGD();
		}

		public	function setGC (gc:int)
		{
			money.gc = gc;
			if(Main.Marketplace.isActiveScreen)
				Main.Marketplace.setNewGC();
			if(Main.CreateSurvPopup.visible)
				Main.CreateSurvPopup.CreateSurvPopup.refreshCharacterUnlock();
		}

		public	function setCells (cells:int)
		{
			money.cells = cells;
		}
		
		public	function buyItemSuccessful ()
		{
			Main.buyItemSuccessful ();
		}
		
		public	function addBackpackItem (survivor:int, slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, description:String = "")
		{
			if (survivor >= 0 && survivor < 5)
			{
				var	s:Survivor = Survivors[survivor];
				
				var	backpackItem:BackpackItem = s.getBackpackItem (slotID);
				
				if (backpackItem)
				{
					backpackItem.quantity += quantity;
					backpackItem.Description = description;
				}
				else		
					s.addBackpackItem (slotID, inventoryID, itemID, quantity, var1, var2, description);
			}
		}
		
		public	function set SelectedChar (char:int):void
		{
			this.curSurvivor = char;
			
			FrontEndEvents.eventSetSelectedChar (char);
		}

		public	function get SelectedChar ():int
		{
			return this.curSurvivor;
		}
		
		public	function changeBackpackSuccess ()
		{
			Main.Inventory.fillInventory (Main.Inventory.SelectedTabID);
			Main.Inventory.showDescription (null);
			Main.Inventory.showBackpack ();
		}
		
		public	function backpackGridSwapSuccess ()
		{
			Main.Inventory.fillInventory (Main.Inventory.SelectedTabID);
			Main.Inventory.showBackpack ();
			Main.Inventory.showDescription ();
		}
		
		public	function backpackToInventorySuccess ()
		{
			Main.Inventory.fillInventory (Main.Inventory.SelectedTabID);
			Main.Inventory.showBackpack ();
			Main.Inventory.showDescription ();
		}
		
		public	function backpackFromInventorySuccess ()
		{
			Main.Inventory.fillInventory (Main.Inventory.SelectedTabID);
			Main.Inventory.showBackpack ();
			Main.Inventory.showDescription ();
		}
		
		public function clearScreenResolutions()
		{
			Opt_ScreenResolutions = new Array();
		}
		
		public function addScreenResolution(res:String)
		{
			Opt_ScreenResolutions.push(res);
		}
		
		public function setBrowseGamesOptions(regus:Boolean, regeu:Boolean, regru:Boolean, regsa:Boolean,
											  filt_gw:Boolean, filt_sh:Boolean, filt_empt:Boolean, filt_full:Boolean, filt_private:Boolean,
											  opt_trac:Boolean, opt_nm:Boolean, opt_ch:Boolean, opt_enable:Boolean,
											  opt_password:Boolean)
		{
			BrowseGames_Region_US = regus;
			BrowseGames_Region_EU = regeu;
			BrowseGames_Region_RU = regru;
			BrowseGames_Region_SA = regsa;
			
			BrowseGames_Filter_Gameworld = filt_gw;
			BrowseGames_Filter_Stronghold = filt_sh;
			BrowseGames_Filter_HideEmpty = filt_empt;
			BrowseGames_Filter_HideFull = filt_full;
			BrowseGames_Filter_PrivateServers = filt_private;
			
			BrowseGames_Options_Tracers = opt_trac;
			BrowseGames_Options_Nameplates = opt_nm;
			BrowseGames_Options_Crosshair = opt_ch;
			BrowseGames_Options_Enable = opt_enable;			
			BrowseGames_Options_Password = opt_password;
		}
		
		public function setOptions(res:Number, ovQ:Number, brightness:Number, contrast:Number, soundV:Number, musicV:Number, commV:Number, hintS:Number, redBlood:Number, 
					AA:Number, ssao:Number, terrD:Number, terrT:Number, waterQ:Number, shadowQ:Number, lightQ:Number, effectQ:Number,
					meshD:Number, aniso:Number, postproc:Number, meshT:Number,
					vertLook:Number, leftSideCam:Number, mouseWheel:Number, MouseSens:Number, MouseAccel:Number, toggleAim:Number, toggleCrouch:Number,
					fullscreen:Number, vsync:Number)
		{
			Opt_ScreenResIndex=res;
			Opt_OverallQ=ovQ;
			Opt_Brightness=brightness;
			Opt_Contrast=contrast;
			Opt_SoundV=soundV;
			Opt_MusicV=musicV;
			//Opt_CommV=commV;
			Opt_hintS=hintS;
			Opt_RedBlood=redBlood;
			Opt_AA=AA;
			Opt_VSync=vsync;
			Opt_SSAO=ssao;
			Opt_TerrainD=terrD;
			Opt_TerrainT=terrT;
			Opt_WaterQ=waterQ;
			Opt_ShadowQ=shadowQ;
			Opt_LightQ=lightQ;
			Opt_EffectsQ=effectQ;
			Opt_MeshD=meshD;
			Opt_Aniso=aniso;
			Opt_PostProc=postproc;
			Opt_MeshT=meshT;
	
			Opt_vertLook=vertLook;
			Opt_leftSideCam=leftSideCam;
			Opt_mouseWheel=mouseWheel;
			Opt_mouseSens=MouseSens;
			Opt_mouseAccel=MouseAccel;
			Opt_toggleAim=toggleAim;
			Opt_toggleCrouch=toggleCrouch;
			
			Opt_Fullscreen = fullscreen;
		}
		
		public function setVoipOptions(enableVoice:Number, showBubble:Number, inputDev:Number, outputDev:Number, voiceVolume:Number)
		{
			Opt_EnableVoiceChat = enableVoice;
			Opt_ShowChatBubble = showBubble;
			Opt_InputDevice = inputDev;
			Opt_OutputDevice = outputDev;
			Opt_VoiceChatVolume = voiceVolume;
		}
		
		public function reloadOptions()
		{
			Main.OptionsControl.resetValues();
			Main.OptionsGeneral.resetValues();
			Main.OptionsVoip.resetValues();
		}
		
		public function addKeyboardMapping(name:String, key:String)
		{
			keyboardMapping.push({name:name, key:key});
		}
	
		public function setKeyboardMapping(name:String, key:String)
		{
			for(var i=0; i<keyboardMapping.length; ++i)
			{
				if(keyboardMapping[i]["name"]==name)
				{
					keyboardMapping[i]["key"] = key;
					break;
				}
			}
		}
	
		public function updateKeyboardMapping(index:Number, key:String)
		{
			keyboardMapping[index]["key"] = key;
			Main.OptionsControl.updateKeyMapping();
		}
		
		public function setLanguage(lang:String):void
		{
			m_Language = lang;
			Main.MainMenu.updateTextLanguages();
		}
	
		public function disableLanguageSelection()
		{
			m_DisableLanguageSelection = true;
		}
		
		public	function addHero (itemID:uint, category:int, name:String, desc:String, icon:String, icon2:String, icon3:String, numHeads:int, numBody:int, numLegs:int)
		{
			var	hero:Hero = new Hero (itemID, category, name, desc, icon, icon2, icon3, numHeads, numBody, numLegs);
			
			HeroDB.push(hero);
		}
		
		public	function setButtonText (button:MovieClip, name:String)
		{
			var format1:TextFormat = new TextFormat();
			format1.letterSpacing = 1;
			var	p:Object = button.parent;
			
			button.Text.text = name;
			
			p.Plate.gotoAndStop(p.name);
			p.PlateRed.gotoAndStop(p.name);
			p.Icon.gotoAndStop(p.name);
			p.Frame.gotoAndStop(p.name);
			
			var symbol:String = "_";
			var symbolPos:Number = button.Text.text.indexOf(symbol);
			
			if (symbolPos > -1)
			{
				button.Text.replaceText(symbolPos,symbolPos + 1," ");
			}
			
			button.Text.setTextFormat(format1);
			
			if(p.name == "Revive_Now" || p.name == "Heal") 
			{
				button.Text.x = (p.Plate.width/2.4 - button.Text.textWidth/2)-98
			} 
			else 
			{
				button.Text.x = (p.Plate.width/2 - button.Text.textWidth/2)-98
			}
		}
		
		public function showCharacterUnlock (charID:int):void
		{
			Main.showCharacterUnlock (charID);
		}
		
		public	function characterPurchaseGCSuccessful ():void
		{
			Main.CreateSurvPopup.refreshCharacterUnlock ();
		}

		public	function clearBackpacks ():void
		{
			BackpackDB = new Array ();
		}
		
		public	function addBackpack (slotID:int, itemID:uint):void
		{
			BackpackDB[slotID] = itemID;
		}

		public	function showChangeBackpack ()
		{
			Main.showChangeBackpack();
		}
		
		public	function updateDeadTimer (seconds:int, percent:int, reviveBtnVis:Boolean)
		{
			Main.updateDeadTimer (seconds, percent, reviveBtnVis);
		}
	}
}
