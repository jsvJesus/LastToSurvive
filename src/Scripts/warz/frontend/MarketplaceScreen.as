package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.net.URLRequest;
	import flash.display.Loader;
	import flash.display.Bitmap;
	import warz.utils.Layout;
	import warz.utils.ImageLoader;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.DisplayObject;
	import caurina.transitions.Tweener;
	import warz.dataObjects.*;

	public class MarketplaceScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var Marketplace:MovieClip;
		public var api:warz.frontend.Frontend=null;
		
		public	var	SelectedTabID:int;
		public	var	SelectedItem:Object;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		private var ScrollerIsDragging2:Boolean;
		private var Scroller2:MovieClip;

		public	var	actualGC:int = 0;
		public	var	actualGold:int = 0;
		public	var	actualCells:int = 0;

		public	var	visualGC:int = 0;
		public	var	visualGold:int = 0;
		public	var	visualCells:int = 0;
		
		private var transactionsData:Array = null;
		
		public function MarketplaceScreen() 
		{
			for(var i=1; i<=8; ++i)
			{
				var Name = "Tab"+i;
				Marketplace[Name].Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				Marketplace[Name].Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				Marketplace[Name].Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			}
			
			Marketplace.Tutorial.visible = false;
			Marketplace.Tutorial.addEventListener(MouseEvent.CLICK, function(evt:Event) {
												  Marketplace.Tutorial.visible = false;
												  });

			Marketplace.Money.BtnGC.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			Marketplace.Money.BtnGC.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			Marketplace.Money.BtnGC.Btn.addEventListener(MouseEvent.CLICK, BuyGCPressFn);
			Marketplace.Money.BtnGC.Icon.gotoAndStop(1);

			Marketplace.Money.BtnGD.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			Marketplace.Money.BtnGD.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			Marketplace.Money.BtnGD.Btn.addEventListener(MouseEvent.CLICK, BuyGDPressFn);
			Marketplace.Money.BtnGD.Icon.gotoAndStop(2);

			Marketplace.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			Marketplace.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			Marketplace.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.CLICK, ShowGCLogPressFn);
			Marketplace.Money.TransactionsBtn.Text.Text.text = "$FR_GC_STATEMENT";

			Marketplace["Tab7"].visible = false;
			Marketplace["Tab8"].visible = false;
			
			Marketplace.PopupTransactions.visible = false;
			Marketplace.PopupTransactions.BtnBack.Text.Text.text = "$FR_Back";
			Marketplace.PopupTransactions.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			Marketplace.PopupTransactions.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			Marketplace.PopupTransactions.BtnBack.Btn.addEventListener(MouseEvent.CLICK, function(evt:Event){
																	SoundEvents.eventSoundPlay("menu_click");
																	evt.currentTarget.parent.gotoAndPlay("pressed");
																	hideTransactionsPopup();
																	   });
			
			
			
			
			Name = "Slot1";
			
			var me = this;
			
			Scroller = Marketplace.Scroller;
			ScrollerIsDragging = false;
			Scroller.Field.alpha = 0.5;
			Scroller.ArrowUp.alpha = 0.5;
			Scroller.ArrowDown.alpha = 0.5;
			Scroller.Trigger.alpha = 0.5;

			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 1; })
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 0.5; })
			Scroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowUp.alpha = 1;})
			Scroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowUp.alpha = 0.5;})
			Scroller.ArrowUp.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(-10);})
			Scroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowDown.alpha = 1;})
			Scroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowDown.alpha = 0.5; })
			Scroller.ArrowDown.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(+10);})
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startY = me.Scroller.Field.y;
				var endY = me.Scroller.Field.height - me.Scroller.Trigger.height;
				me.ScrollerIsDragging = true;
				me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
			})

			Scroller2 = Marketplace.PopupTransactions.Scroller;
			ScrollerIsDragging2 = false;
			Scroller2.Field.alpha = 0.5;
			Scroller2.ArrowUp.alpha = 0.5;
			Scroller2.ArrowDown.alpha = 0.5;
			Scroller2.Trigger.alpha = 0.5;

			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.ScrollerIsDragging2==true) return; me.Scroller2.Trigger.alpha = 1; })
			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.ScrollerIsDragging2==true) return; me.Scroller2.Trigger.alpha = 0.5; })
			Scroller2.ArrowUp.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller2.ArrowUp.alpha = 1;})
			Scroller2.ArrowUp.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller2.ArrowUp.alpha = 0.5;})
			Scroller2.ArrowUp.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown2(-10);})
			Scroller2.ArrowDown.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller2.ArrowDown.alpha = 1;})
			Scroller2.ArrowDown.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller2.ArrowDown.alpha = 0.5; })
			Scroller2.ArrowDown.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown2(+10);})
			
			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startY = me.Scroller2.Field.y;
				var endY = me.Scroller2.Field.height - me.Scroller2.Trigger.height;
				me.ScrollerIsDragging2 = true;
				me.Scroller2.Trigger.startDrag(false, new Rectangle (me.Scroller2.Trigger.x, startY, 0, endY));
			})
		}
		
		public function handleScrollerUpDown(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = Scroller.Field.y; 
				if(Scroller.Trigger.y > startY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y < startY) 
						Scroller.Trigger.y = startY; 
				} 
			}
			else
			{
				var endY = Scroller.Field.height - Scroller.Trigger.height + 26;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}
		public function handleScrollerUpDown2(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = Scroller2.Field.y; 
				if(Scroller2.Trigger.y > startY) { 
					Scroller2.Trigger.y += delta; 
					if(Scroller2.Trigger.y < startY) 
						Scroller2.Trigger.y = startY; 
				} 
			}
			else
			{
				var endY = Scroller2.Field.height - Scroller2.Trigger.height + 26;
				if(Scroller2.Trigger.y < endY) { 
					Scroller2.Trigger.y += delta; 
					if(Scroller2.Trigger.y > endY) 
						Scroller2.Trigger.y = endY; 
				} 
			}
		}

		// event functions
		public	function BuyGCRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function BuyGCRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function ShowGCLogPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
				
			evt.currentTarget.parent.gotoAndPlay("pressed");
			transactionsData = new Array();
			FrontEndEvents.eventRequestGCTransactionData();
			if(api.isDebug)
			{
				addTransactionData(1, "1/1/2013", "test1", "+45.43", "1233.12");
				for(var i=1;i<50; ++i)
					addTransactionData(i+1, "12/12/2013", "test10", "-45.43", "-1233.12");					
				addTransactionData(100, "31/12/2013", "test99", "+45.43", "1233.12");
				showTransactionsPopup();
			}
		}		
		
		public function addTransactionData(id:int, date:String, name:String, amount:String, balance:String)
		{
			transactionsData.push({id:id, date:date, name:name, amount:amount, balance:balance});
		}
		
		public function showTransactionsPopup()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel2);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp2);

			while (Marketplace.PopupTransactions.Slots.numChildren > 0)
			{
				Marketplace.PopupTransactions.Slots.removeChildAt(0);
			}			
			
			var y = 0;
			for(var i=0; i<transactionsData.length; ++i)
			{
				var slot = new warz.frontend.MarketplaceTransactionsSlot();
				slot.x = 0;
				slot.y = y;
				slot.Num.text = transactionsData[i]["id"];
				slot.Date.text = transactionsData[i]["date"];
				slot.Transaction.text = transactionsData[i]["name"];
				slot.SpentEarned.text = transactionsData[i]["amount"];
				slot.Balance.text = transactionsData[i]["balance"];
				
				Marketplace.PopupTransactions.Slots.addChild(slot);
				
				y+=48;
			}

			if (transactionsData.length > 7)
			{
				Scroller2.visible = true;
				var me = this;
				
				Scroller2.Trigger.y = Scroller2.Field.y;
				Scroller2.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems2);
				
				Scroller2.ArrowDown.y = Marketplace.PopupTransactions.Mask.height - 4;
				Scroller2.Field.height = (Marketplace.PopupTransactions.Mask.height - (Scroller2.ArrowDown.height * 2));
			}
			else
			{
				Scroller2.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems2);
				Scroller2.visible = false;
				Marketplace.PopupTransactions.Slots.y = 329;
			}
			
			Marketplace.PopupTransactions.visible = true;
		}
		public function hideTransactionsPopup()
		{
			Scroller2.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems2);

			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel2);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp2);

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
			
			Marketplace.PopupTransactions.visible = false;
		}
		
		public	function BuyGCPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
				
			evt.currentTarget.parent.gotoAndPlay("pressed");
			FrontEndEvents.eventStorePurchaseGPRequest();
			//api.Main.PurchaseGC.showPopUp();
		}		

		public	function BuyGDPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
				
			evt.currentTarget.parent.gotoAndPlay("pressed");
			api.Main.PurchaseGC.showPopUp(false);
		}		

		public	function SlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function SlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function SlotPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if (evt.currentTarget.parent.name != "BuyBtn")
				{
					for (var i:int = 0; i < Marketplace.Slots.numChildren; i++)
					{
						var	obj = Marketplace.Slots.getChildAt(i);
						
						if(obj.State == "active")
						{
							obj.State = "off";
							obj.gotoAndPlay("out");
						}					
					}
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as MarketplaceScreen))
				{
					p = p.parent as MovieClip;
				}
				
				(p as MarketplaceScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			FrontEndEvents.eventMarketplaceActive();

			this.gotoAndPlay("start");
			
			var	tabs:Array = api.getStoreTabs ();

			for(var i=1; i<=8; ++i)
			{
				var Name = "Tab"+i;
				var	tab:Object = tabs[i - 1];

				Marketplace[Name].State = "off";
				Marketplace[Name].gotoAndPlay("out");
				
				if (tab && 
					Marketplace[Name].Icon.currentLabel != tab.name)
				{
					Marketplace[Name].Icon.gotoAndPlay(tab.name);
					Marketplace[Name].IconAdd.gotoAndPlay(tab.name);
				}
			}
			
			Marketplace["Tab1"].State = "active";
			Marketplace["Tab1"].gotoAndPlay("pressed");

			visualGC = actualGC = api.money.gc;
			visualGold = actualGold = api.money.dollars;
			visualCells = actualCells = api.money.cells;

			updateStoreItemsList (0);
			showDescription (null);
			updateGC ();
			updateGold ();
			updateCells ();

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function Deactivate()
		{
			this.mouseEnabled = false;
			this.gotoAndPlay("end");
			
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > Marketplace.Mask.x && 
				e.stageX < Marketplace.Mask.x + Marketplace.Mask.width + 45 &&
				e.stageY > Marketplace.Mask.y && 
				e.stageY < Marketplace.Mask.y + Marketplace.Mask.height)
			{
				var	dist:Number = (Marketplace.Slots.height - Marketplace.Mask.height) / 177;
				var	h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}

		public	function mouseUp2(e:Event) :void
		{ 
			ScrollerIsDragging2 = false;
			Scroller2.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel2 (e:MouseEvent):void
		{
			if (e.stageX > Marketplace.PopupTransactions.Mask.x && 
				e.stageX < Marketplace.PopupTransactions.Mask.x + Marketplace.PopupTransactions.Mask.width + 45 &&
				e.stageY > Marketplace.PopupTransactions.Mask.y && 
				e.stageY < Marketplace.PopupTransactions.Mask.y + Marketplace.PopupTransactions.Mask.height)
			{
				var	dist:Number = (Marketplace.PopupTransactions.Slots.height - Marketplace.PopupTransactions.Mask.height) / 48;
				var	h = Scroller2.Field.height - Scroller2.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown2(-dist);
				else
					handleScrollerUpDown2(dist);
			}
		}

		private function updateStoreItemsList(tabID:Number)
		{
			SelectedTabID = tabID;
			
			var storeDB:Array = api.StoreDB;
			var inventoryDB:Array = api.InventoryDB;
			var sortedItemDB:Array = new Array();
			var item:Item = null;
			var	storeItem:StoreItem;
			var	Name = "";
			var	t;
			var	tabs:Array = api.getStoreTabs ();
			
			for(var i = 0; i < storeDB.length; i++)
			{
				storeItem = storeDB[i];
				item = api.getItemByID(storeItem.itemID);
				
				var	tab = tabs[SelectedTabID];
				
				if (!tab)
					continue;
				
				for (var c = 0; c < tab["categories"].length; c++)
				{
					var	cat	= tab["categories"][c]
					
					if(item && item.catID == cat["catID"])
					{
						sortedItemDB.push({item:item, storeItem:storeItem});
					}
				}
			}
			
			while (Marketplace.Slots.numChildren > 0)
			{
				Marketplace.Slots.removeChildAt(0);
			}
			Marketplace.Slots.y = 284.45;

			var inventItem = null;
			
			var slotOffsetX:int = 2;
			var	slotOffsetY:int = 2;
			var	slotWidth:int = 358;
			var	slotHeight:int = 177;

			for(i = 0; i < sortedItemDB.length; i++)
			{
				item = sortedItemDB[i]["item"];
				storeItem = sortedItemDB[i]["storeItem"];
				
				var storeSlot = new warz.frontend.MarketplaceSlot();
				storeSlot.name = "PopupSlot" + (i + 1);
				storeSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				storeSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				storeSlot.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
				storeSlot.x = slotOffsetX + (i % 3) * slotWidth;
				storeSlot.y = slotOffsetY + int (i / 3) * slotHeight;
				
				if (storeSlot.Pic.numChildren > 0)
					storeSlot.Pic.removeChildAt(0);

				Marketplace.Slots.addChild(storeSlot);
				
				loadSlotIcon  (item.Icon, storeSlot.Pic, 1);
				
				storeSlot.Text.visible = true;
				storeSlot.Text.Text.text = item.Name;
				
/*				inventItem = api.getInventoryItemByID(item.itemID);
				
				if (inventItem)
				{
					storeSlot.alpha = 0.5;
					storeSlot.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
					storeSlot.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
					storeSlot.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
				}
*/								
				if (storeSlot.RentNum)
				{
					storeSlot.RentNum.visible = false;
//					storeSlot.RentNum.Rent.text = "$FR_MARKETPLACE_PERMANENT";
				}
				
				if (storeSlot.NumNum)
				{
					if (storeItem.quantity > 1)
					{
						storeSlot.NumNum.visible = true;
						storeSlot.NumNum.Num.text = String (storeItem.quantity);
					}
					else
					{
						storeSlot.NumNum.visible = false;
					}
				}
				
				storeSlot.New.visible = storeItem.newItem;

				if (storeItem.price != 0)
				{
					if (storeSlot.Text.currentLabel != "gc")
						storeSlot.Text.gotoAndPlay("gc");
						
					storeSlot.Text.Price.text = storeItem.price;
				}
				else
				{
					if (storeSlot.Text.currentLabel != "gold")
						storeSlot.Text.gotoAndPlay("gold");
						
					storeSlot.Text.Price.text = storeItem.priceGD;
				}
				
				storeSlot.Item = item;
				storeSlot.StoreItem = storeItem;
			}
			
			if (sortedItemDB.length > 12)
			{
				Scroller.visible = true;
				var me = this;
				
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = Marketplace.Mask.height - 4;
				Scroller.Field.height = (Marketplace.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				Marketplace.Slots.y = 284.45;
			}
			
		}
		
		public function loadSlotIcon(path:String, imageHolder:MovieClip, slotType:Number)
		{
			var	dat:Object = {imageHolder:imageHolder, slotType:slotType};
			
			var	imageLoader:ImageLoader = new ImageLoader (path, loadSlotCallback, dat)
		}
		
		public	function loadSlotCallback (bitmap:Bitmap, dat:Object)
		{
			var slotWidth=0, slotHeight=0;
			var slotX=0, slotY=0;
			
			if (dat.slotType == 1)
			{
				slotWidth = 256;
				slotHeight = 164;
				slotX = 130;
				slotY = 80;
			}
			else if (dat.slotType == 2)
			{
				slotWidth = 513;
				slotHeight = 329;
				slotX = 270;
				slotY = 160;
			}

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight,  bitmap.width, bitmap.height,"uniform");
			
			bitmap.x += slotX; 
			bitmap.y += slotY; 
		
			dat.imageHolder.scaleX = dat.imageHolder.scaleY = 1;
			dat.imageHolder.addChild (bitmap);
		}
		
		public	function ActionFunction (button:String)
		{
			if (button == "BuyBtn")
			{
				Marketplace.BuyBtn.State = "off";
				Marketplace.BuyBtn.gotoAndPlay("out");
				
				if (SelectedItem)
				{
					var	item:Item = SelectedItem.Item;
					var	storeItem:StoreItem = SelectedItem.StoreItem;
					
					if (api.isDebug)
					{
						var	inventItem:InventoryItem = api.getInventoryItemByID (item.itemID);

						if (inventItem)
						{
							inventItem.quantity += 1;
						}
						else
							api.addInventoryItem(api.InventoryDB.length, item.itemID, 1, 0, 0, false);
							
						api.money.gc -= storeItem.price;
						api.money.dollars -= storeItem.priceGD;
							
						api.buyItemSuccessful ()
					}
					else 
						FrontEndEvents.eventBuyItem (item.itemID, storeItem.price, storeItem.priceGD);
				}
			}
			else if (button.indexOf("PopupSlot") != -1)
			{
				var	slot = int (button.slice (9)) - 1;
				var storeSlot = Marketplace.Slots.getChildAt(slot);

				if (storeSlot)
				{
					showDescription (storeSlot);
				}
			}
			else if (button.indexOf("Tab") != -1)
			{
				var	tab = int (button.slice (3)) - 1;
				var Name = "Tab" + (SelectedTabID + 1);
				Marketplace[Name].State = "off";
				Marketplace[Name].gotoAndPlay("out");

				updateStoreItemsList (tab);
				showDescription (null);
			}
		}
		
		public	function showDescription (slot)
		{
			var inventItem = null;
			
			Marketplace.BuyBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Marketplace.BuyBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Marketplace.BuyBtn.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
			
			if (slot == null || slot.Item == null)
			{
				SelectedItem = null;
				
				Marketplace.DescrBlock.Name.text = "";
				if (Marketplace.DescrBlock.Pic.numChildren > 0)
					Marketplace.DescrBlock.Pic.removeChildAt(0);
					
				Marketplace.DescrBlock.DescrText.text = "";
				Marketplace.DescrBlock.Price.visible = false;
				Marketplace.DescrBlock.Stack.visible = false;
				
				Marketplace.BuyBtn.visible = false;
			}
			else
			{
				SelectedItem = slot;
				
				var	item:Item = SelectedItem.Item;
				var	storeItem:StoreItem = SelectedItem.StoreItem;
				
				if (Marketplace.DescrBlock.Pic.numChildren > 0)
					Marketplace.DescrBlock.Pic.removeChildAt(0);
					
				Marketplace.DescrBlock.Name.text = item.Name;
				Marketplace.DescrBlock.DescrText.text = item.desc;
				Marketplace.DescrBlock.Price.visible = true;
				Marketplace.DescrBlock.Stack.visible = true;
				Marketplace.DescrBlock.Stack.StackNum.text = storeItem.quantity;

				loadSlotIcon  (item.Icon, Marketplace.DescrBlock.Pic, 2);
				
				if (storeItem.price != 0)
				{
					if (Marketplace.DescrBlock.Price.currentLabel != "gc")
						Marketplace.DescrBlock.Price.gotoAndPlay("gc");
						
					Marketplace.DescrBlock.Price.Price.text = storeItem.price;

					if (Marketplace.BuyBtn.Text.currentLabel != "gc")
						Marketplace.BuyBtn.Text.gotoAndPlay("gc");

					Marketplace.BuyBtn.Text.Text.text = "$FR_MARKETPLACE_BUY_FOR " + storeItem.price;
				}
				else
				{
					if (Marketplace.DescrBlock.Price.currentLabel != "gold")
						Marketplace.DescrBlock.Price.gotoAndPlay("gold");
					Marketplace.DescrBlock.Price.Price.text = storeItem.priceGD;

					if (Marketplace.BuyBtn.Text.currentLabel != "gold")
						Marketplace.BuyBtn.Text.gotoAndPlay("gold");

					Marketplace.BuyBtn.Text.Text.text = "$FR_MARKETPLACE_BUY_FOR " + storeItem.priceGD;
				}

				Marketplace.BuyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				Marketplace.BuyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				Marketplace.BuyBtn.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
				Marketplace.BuyBtn.visible = true;
			}
		}

		public function setNewGC()
		{
			actualGC = api.money.gc;
			Tweener.addTween(this, {visualGC:actualGC, time:0.5, transition:"linear", onUpdate:updateGC});
		}

		public function setNewGD()
		{
			actualGold = api.money.dollars;
			Tweener.addTween(this, {visualGold:actualGold, time:0.5, transition:"linear", onUpdate:updateGold});
		}

		public function buyItemSuccessful ()
		{
			actualGC = api.money.gc;
			actualGold = api.money.dollars;
			actualCells = api.money.cells;

			Tweener.addTween(this, {visualGC:actualGC, time:0.5, transition:"linear", onUpdate:updateGC});
			Tweener.addTween(this, {visualGold:actualGold, time:0.5, transition:"linear", onUpdate:updateGold});
			Tweener.addTween(this, {visualCells:actualCells, time:0.5, transition:"linear", onUpdate:updateCells});
		}
		
		public	function updateGC ()
		{
			Marketplace.Money.BtnGC.Text.Text.text = visualGC;
		}

		public	function updateGold ()
		{
			Marketplace.Money.BtnGD.Text.Text.text = visualGold;
		}
		
		public	function updateCells ()
		{
			Marketplace.Money.Cells.text = visualCells;
		}

		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = Marketplace.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = Marketplace.Mask.height;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 173);
			
			if (Marketplace.Slots.y != (284.45 + (174 * step)))
			{
				Tweener.addTween(Marketplace.Slots, {y:(284.45 + (174 * step)), time:0.25, transition:"linear"});
			}
		}
		
		public	function scrollItems2(e:Event)
		{
			var	a = Scroller2.Field.y;
			var	b = Scroller2.Trigger.y;
			var	dist = (Scroller2.Field.y - Scroller2.Trigger.y);
			var	h = Marketplace.PopupTransactions.Slots.height;
			var	h1 = Scroller2.Field.height - Scroller2.Trigger.height;
			var	mh = Marketplace.PopupTransactions.Mask.height;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 48);
			
			if (Marketplace.PopupTransactions.Slots.y != (329+(48 * step)))
			{
				Tweener.addTween(Marketplace.PopupTransactions.Slots, {y:329+(48 * step), time:0.25, transition:"linear"});
			}
		}
	}
}
