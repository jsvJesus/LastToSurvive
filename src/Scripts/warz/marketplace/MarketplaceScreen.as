package warz.marketplace {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.net.URLRequest;
	import flash.display.Loader;
	import flash.display.Bitmap;
	import warz.utils.Layout;
	import warz.utils.ImageLoader;
	import warz.events.MarketEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.DisplayObject;
	import caurina.transitions.Tweener;
	import warz.dataObjects.*;

	public class MarketplaceScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.marketplace.Market=null;
		
		public	var	SelectedTabID:int;
		public	var	SelectedItem:Object;
		
		private var ScrollerIsDragging:Boolean;
		public var Scroller:MovieClip;

		public	var	actualGC:int = 0;
		public	var	actualGold:int = 0;
		public	var	actualCells:int = 0;

		public	var	visualGC:int = 0;
		public	var	visualGold:int = 0;
		public	var	visualCells:int = 0;
		
		public function MarketplaceScreen() 
		{
			this.visible = false;

			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			this.BtnBack.Text.Text.text = "$HUD_Pause_ReturnToGame";

			for(var i=1; i<=8; ++i)
			{
				var Name = "Tab"+i;
				this[Name].Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				this[Name].Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				this[Name].Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			}

			this.Money.BuyGCBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			this.Money.BuyGCBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			this.Money.BuyGCBtn.Btn.addEventListener(MouseEvent.CLICK, BuyGCPressFn);
			this.Money.BuyGCBtn.Text.Text.text = "$FR_CREATE_CHARACTER_PURCHASE_GC";
			this.Money.BuyGCBtn.visible = false;

			this.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BuyGCRollOverFn);
			this.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BuyGCRollOutFn);
			//this.Money.TransactionsBtn.Btn.addEventListener(MouseEvent.CLICK, ShowGCLogPressFn);
			this.Money.TransactionsBtn.Text.Text.text = "$FR_GC_STATEMENT";
			this.Money.TransactionsBtn.visible = false;

			this["Tab7"].visible = false;
			this["Tab8"].visible = false;
			
			Name = "Slot1";
			
			var me = this;
			
			//Scroller = Marketplace.Scroller;
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
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:MouseEvent) 
			{
				if(evt.buttonDown){
					var startY = me.Scroller.Field.y;
					var endY = me.Scroller.Field.height - me.Scroller.Trigger.height;
					me.ScrollerIsDragging = true;
					me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
				}
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
		// event functions
		public	function BuyGCRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function BuyGCRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function BuyGCPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
				
			evt.currentTarget.parent.gotoAndPlay("pressed");
			//MarketEvents.eventStorePurchaseGPRequest();
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
					for (var i:int = 0; i < this.Slots.numChildren; i++)
					{
						var	obj = this.Slots.getChildAt(i);
						
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
			this.visible = true;
			
			var	tabs:Array = api.getStoreTabs ();

			for(var i=1; i<=8; ++i)
			{
				var Name = "Tab"+i;
				var	tab:Object = tabs[i - 1];

				this[Name].State = "off";
				this[Name].gotoAndPlay("out");
				
				if (tab && 
					this[Name].Icon.currentLabel != tab.name)
				{
					this[Name].Icon.gotoAndPlay(tab.name);
					this[Name].IconAdd.gotoAndPlay(tab.name);
				}
			}
			
			this["Tab1"].State = "active";
			this["Tab1"].gotoAndPlay("pressed");

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
			this.visible = false;
			this.mouseEnabled = false;
			
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
			if (e.stageX > this.Mask.x && 
				e.stageX < this.Mask.x + this.Mask.width + 45 &&
				e.stageY > this.Mask.y && 
				e.stageY < this.Mask.y + this.Mask.height)
			{
				var	dist:Number = (this.Slots.height - this.Mask.height) / 177;
				var	h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}

		private function updateStoreItemsList(tabID:Number)
		{
			SelectedTabID = tabID;
			
			var storeDB:Array = api.StoreDB;
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
			
			while (this.Slots.numChildren > 0)
			{
				this.Slots.removeChildAt(0);
			}
			this.Slots.y = 284.45;

			var inventItem = null;
			
			var slotOffsetX:int = 2;
			var	slotOffsetY:int = 2;
			var	slotWidth:int = 358;
			var	slotHeight:int = 177;

			for(i = 0; i < sortedItemDB.length; i++)
			{
				item = sortedItemDB[i]["item"];
				storeItem = sortedItemDB[i]["storeItem"];
				
				var storeSlot = new warz.marketplace.MarketplaceSlot();
				storeSlot.name = "PopupSlot" + (i + 1);
				storeSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				storeSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				storeSlot.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
				storeSlot.x = slotOffsetX + (i % 3) * slotWidth;
				storeSlot.y = slotOffsetY + int (i / 3) * slotHeight;
				
				if (storeSlot.Pic.numChildren > 0)
					storeSlot.Pic.removeChildAt(0);

				this.Slots.addChild(storeSlot);
				
				loadSlotIcon  (item.Icon, storeSlot.Pic, 1);
				
				storeSlot.Text.visible = true;
				storeSlot.Text.Text.text = item.Name;
				
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
				
				Scroller.ArrowDown.y = this.Mask.height - 4;
				Scroller.Field.height = (this.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				this.Slots.y = 284.45;
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
				this.BuyBtn.State = "off";
				this.BuyBtn.gotoAndPlay("out");
				
				if (SelectedItem)
				{
					var	item:Item = SelectedItem.Item;
					var	storeItem:StoreItem = SelectedItem.StoreItem;
					
					if (api.isDebug)
					{
						api.money.gc -= storeItem.price;
						api.money.dollars -= storeItem.priceGD;
							
						api.buyItemSuccessful ()
					}
					else 
						MarketEvents.eventBuyItem (item.itemID, storeItem.price, storeItem.priceGD);
				}
			}
			else if (button == "BtnBack")
			{
				this.BtnBack.State = "off";
				this.BtnBack.gotoAndPlay("out");

				MarketEvents.eventReturnToGame();
			}
			else if (button.indexOf("PopupSlot") != -1)
			{
				var	slot = int (button.slice (9)) - 1;
				var storeSlot = this.Slots.getChildAt(slot);

				if (storeSlot)
				{
					showDescription (storeSlot);
				}
			}
			else if (button.indexOf("Tab") != -1)
			{
				var	tab = int (button.slice (3)) - 1;
				var Name = "Tab" + (SelectedTabID + 1);
				this[Name].State = "off";
				this[Name].gotoAndPlay("out");

				updateStoreItemsList (tab);
				showDescription (null);
			}
		}
		
		public	function showDescription (slot)
		{
			var inventItem = null;
			
			this.BuyBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			this.BuyBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			this.BuyBtn.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
			
			if (slot == null || slot.Item == null)
			{
				SelectedItem = null;
				
				this.DescrBlock.Name.text = "";
				if (this.DescrBlock.Pic.numChildren > 0)
					this.DescrBlock.Pic.removeChildAt(0);
					
				this.DescrBlock.DescrText.text = "";
				this.DescrBlock.Price.visible = false;
				this.DescrBlock.Stack.visible = false;
				
				this.BuyBtn.visible = false;
			}
			else
			{
				SelectedItem = slot;
				
				var	item:Item = SelectedItem.Item;
				var	storeItem:StoreItem = SelectedItem.StoreItem;
				
				if (this.DescrBlock.Pic.numChildren > 0)
					this.DescrBlock.Pic.removeChildAt(0);
					
				this.DescrBlock.Name.text = item.Name;
				this.DescrBlock.DescrText.text = item.desc;
				this.DescrBlock.Price.visible = true;
				this.DescrBlock.Stack.visible = true;
				this.DescrBlock.Stack.StackNum.text = storeItem.quantity;

				loadSlotIcon  (item.Icon, this.DescrBlock.Pic, 2);
				
				if (storeItem.price != 0)
				{
					if (this.DescrBlock.Price.currentLabel != "gc")
						this.DescrBlock.Price.gotoAndPlay("gc");
						
					this.DescrBlock.Price.Price.text = storeItem.price;

					if (this.BuyBtn.Text.currentLabel != "gc")
						this.BuyBtn.Text.gotoAndPlay("gc");

					this.BuyBtn.Text.Text.text = "$FR_MARKETPLACE_BUY_FOR " + storeItem.price;
				}
				else
				{
					if (this.DescrBlock.Price.currentLabel != "gold")
						this.DescrBlock.Price.gotoAndPlay("gold");
					this.DescrBlock.Price.Price.text = storeItem.priceGD;

					if (this.BuyBtn.Text.currentLabel != "gold")
						this.BuyBtn.Text.gotoAndPlay("gold");

					this.BuyBtn.Text.Text.text = "$FR_MARKETPLACE_BUY_FOR " + storeItem.priceGD;
				}

				this.BuyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				this.BuyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				this.BuyBtn.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
				this.BuyBtn.visible = true;
			}
		}

		public function setNewGC()
		{
			actualGC = api.money.gc;
			Tweener.addTween(this, {visualGC:actualGC, time:0.5, transition:"linear", onUpdate:updateGC});
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
			this.Money.GC.text = visualGC;
		}

		public	function updateGold ()
		{
			this.Money.Gold.text = visualGold;
		}
		
		public	function updateCells ()
		{
			this.Money.Cells.text = visualCells;
		}

		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = this.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = this.Mask.height;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 173);
			
			if (this.Slots.y != (284.45 + (174 * step)))
			{
				Tweener.addTween(this.Slots, {y:(284.45 + (174 * step)), time:0.25, transition:"linear"});
			}
		}
	}
}
