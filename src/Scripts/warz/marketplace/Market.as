package warz.marketplace  {
	import flash.events.*;
	import flash.display.MovieClip;
	import flash.net.URLRequest;
	import flash.display.Loader;
	import warz.utils.Layout;
	import flash.display.Bitmap;
	import caurina.transitions.Tweener;
	import flashx.textLayout.formats.Category;
	import warz.events.MarketEvents;
	import flash.text.TextFormat;
	import flash.text.TextField;
	import scaleform.gfx.Extensions;
	import warz.dataObjects.*;
	
	public class Market  {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;
		
		static public var api:Market = null;
		
		public var tweenDelay:Number = 0.2;
	
		public 	var survivor:Survivor;
		public	var	CategoryDB:Array = null;
		public	var	PackageDB:Array;
		public	var	ItemDB:Array = null;
		public	var	StoreDB:Array = null;
		public	var	TabDB:Array = null;
		public	var	SlotDB:Array = null;
		
// basic stats
		public	var	money:Object;

		public function Market(main:MovieClip) 
		{
			api = this;
			Main = main;
			Main.api = api;
			
			scaleform.gfx.Extensions.enabled = true;
			
			Tweener.init();
			
			money = {dollars:0, gc:0, cells:0};
			
			CategoryDB = new Array ();
			TabDB = new Array ();
			ItemDB = new Array ();
			StoreDB = new Array ();
			PackageDB = new Array ();
			SlotDB = new Array ([], [], [], [], []);
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
		
		public function showMarketplace()
		{
			Main.Activate();
		}

		public function hideMarketplace()
		{
			Main.Deactivate();
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
		
		public	function addHero (itemID:uint, category:int, name:String, desc:String, icon:String, icon2:String, icon3:String, numHeads:int, numBody:int, numLegs:int)
		{
			// empty to remove error on the game side
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
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			Main.MsgBox.showInfoMsg (msg, OKButton, title);
		}
		
		public function hideInfoMsg()
		{
			Main.MsgBox.visible = false; 
		}
		
		public	function setDollars (dollars:int)
		{
			money.dollars = dollars;
		}

		public	function setGC (gc:int)
		{
			money.gc = gc;
			Main.setNewGC();
		}

		public	function setCells (cells:int)
		{
			money.cells = cells;
		}
		
		public	function buyItemSuccessful ()
		{
			Main.buyItemSuccessful ();
		}
	}
}
