package warz.safelock {
	import flash.display.MovieClip;
	import caurina.transitions.Tweener;
	import warz.dataObjects.Backpack;
	import warz.dataObjects.Survivor;
	import warz.dataObjects.Item;
	import warz.dataObjects.InventoryItem;
	import warz.dataObjects.BackpackItem;
	import flash.events.MouseEvent;
	import warz.events.SafelockEvents;
	import scaleform.gfx.Extensions;
	
	public class safelock extends MovieClip {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;

		static public var api:safelock = null;
		
		public var tweenDelay:Number = 0.2;
	
		public 	var survivor:Survivor;
		public	var	CategoryDB:Array = null;
		public	var	InventoryDB:Array = null;
		public	var	PackageDB:Array;
		public	var	ItemDB:Array = null;
		public	var	StoreDB:Array = null;
		public	var	TabDB:Array = null;
		public	var	SlotDB:Array = null;
		public	var	HeroDB:Array = null;
		public	var	BackpackDB:Array = null;
		
		public function safelock(main:MovieClip) 
		{
			api = this;
			Main = main;
			
			Main.api = api;
			
			scaleform.gfx.Extensions.enabled = true;
			
			Tweener.init();
			
			CategoryDB = new Array ();
			TabDB = new Array ();
			BackpackDB = new Array ();
			InventoryDB = new Array ();
			ItemDB = new Array ();
			StoreDB = new Array ();
			PackageDB = new Array ();
			HeroDB = new Array ();
			SlotDB = new Array ([], [], [], [], []);
		}

		public function showInventoryScreen()
		{
			Main.Activate();
		}
		
		public function hideInventoryScreen()
		{
			Main.Deactivate();
		}

		public function clearInventory()
		{
			InventoryDB = new Array();
		}
		
		public function clearBackpack()
		{
			survivor.clearBackpack ();
		}
		
		public	function updateClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize)
		{
				if (survivor.Name == Name)
				{
					survivor.health = health;
					survivor.xp = xp;
					survivor.timeplayed = timeplayed;
					survivor.hunger = hunger;
					survivor.alive = alive;
					survivor.thirst = thirst;
					survivor.toxic = toxic;
					
					if (survivor.backpackItem.itemID != backpackID)
					{
						var	backpack:Item = getItemByID(backpackID);
						
						survivor.setBackpack (backpack, backpackSize);
					}
				}
		}
		
		public function updateClientSurvivorWeight(Name:String, weight:Number)
		{
			survivor.weight = weight;
		}
		
		public	function addClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											hardcore:int, heroID:int, headID:int, bodyID:int, legsID:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize:int, weight:Number,
											zombiesKilled:int, banditsKilled:int, civiliansKilled:int,
											alignment:String, lastMap:String, globalInventory:Boolean)
		{
			if(alive == 0)
				health = 0;
			survivor = new Survivor (Name, health, xp, timeplayed, hardcore, heroID, headID, bodyID, legsID, 
												  alive, hunger, thirst, toxic, weight, 
												  zombiesKilled, banditsKilled, civiliansKilled, alignment, lastMap, globalInventory, 0);
												  
			var	backpack:Item = getItemByID(backpackID);
			
			if (backpack)
			{
				survivor.setBackpack (backpack, backpackSize);
			}
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
				SlotDB[slotID2].push (cat);			
				
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
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			Main.MsgBox.showInfoMsg (msg, OKButton, title);
		}
		
		public function hideInfoMsg()
		{
			Main.MsgBox.visible = false; 
		}
		
		public	function addBackpackItem (slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, description:String = "")
		{
				var	backpackItem:BackpackItem = survivor.getBackpackItem (slotID);
				
				if (backpackItem)
				{
					backpackItem.quantity += quantity;
					backpackItem.Description = description;
				}
				else		
					survivor.addBackpackItem (slotID, inventoryID, itemID, quantity, var1, var2, description);
		}
		
		public	function changeBackpackSuccess ()
		{
			Main.fillInventory (0);
			Main.showDescription (null);
			Main.showBackpack ();
		}
		
		public	function backpackGridSwapSuccess ()
		{
			Main.fillInventory (0);
			Main.showBackpack ();
			Main.showDescription ();
		}
		
		public	function backpackToInventorySuccess ()
		{
			Main.fillInventory (0);
			Main.showBackpack ();
			Main.showDescription ();
		}
		
		public	function backpackFromInventorySuccess ()
		{
			Main.fillInventory (0);
			Main.showBackpack ();
			Main.showDescription ();
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
	}
}