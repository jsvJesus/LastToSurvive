package warz.safelock {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.SafelockEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import scaleform.gfx.MouseEventEx;
	import caurina.transitions.Tweener;
	
	public class Inventory extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.safelock.safelock=null;
		
		private	var	inventorySlots:MovieClip;
		private	var	backpackSlots:MovieClip;
		private	var	backpackSlotMasks:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		public var Scroller:MovieClip;
		
		private var BackpackScrollerIsDragging:Boolean;
		public	var	BackpackScroller:MovieClip;

		public	var	ItemIsDragging:Boolean = false;
		public	var	DraggedItem:Object;
		public	var	DragMovie:MovieClip;

		private var isShiftKeyDown:Boolean = false;
		private var MoveItem_inventoryID;
		private var MoveItem_slot;
		private var MoveItem_Quantity;

		public function Inventory() {
			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			this.BtnBack.Text.Text.text = "$FR_Back";

			this.BtnChange.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			this.BtnChange.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			this.BtnChange.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			this.BtnChange.Text.Text.text = "$FR_Safelock_ChangePass";

			this.BtnPickup.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			this.BtnPickup.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			this.BtnPickup.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			this.BtnPickup.Text.Text.text = "$FR_Safelock_Pickup";

			inventorySlots = new MovieClip ();
			inventorySlots.mask = this.Mask;				
			this.addChild(inventorySlots);
			
			backpackSlots = new MovieClip ();
			this.addChild(backpackSlots);
			
			backpackSlotMasks = new MovieClip ();
			backpackSlotMasks.mask = this.BackpackMask;
			this.addChild(backpackSlotMasks);
			
			DragMovie = new MovieClip ();
			this.addChild(DragMovie);
			
			this.HeaderType.text = "$FR_PAUSE_INVENTORY_BACKPACK_TYPE";
			this.HeaderMaxWeight.text = "$FR_PAUSE_INVENTORY_MAX_WEIGHT";
			this.HeaderWeight.text = "$FR_PAUSE_INVENTORY_WEIGHT";
			
			var me = this;

			//Scroller = this.Scroller;
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
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, scrollerHandler); 
			function scrollerHandler(e:MouseEvent):void{
				if(e.buttonDown){
					var startY = me.Scroller.Field.y;
					var endY = me.Scroller.Field.height - me.Scroller.Trigger.height;
					me.ScrollerIsDragging = true;
					me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
				}
			}
			
			//BackpackScroller = this.BackpackScroller;
			BackpackScrollerIsDragging = false;
			BackpackScroller.Field.alpha = 0.5;
			BackpackScroller.ArrowUp.alpha = 0.5;
			BackpackScroller.ArrowDown.alpha = 0.5;
			BackpackScroller.Trigger.alpha = 0.5;

			BackpackScroller.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.BackpackScrollerIsDragging==true) return; me.BackpackScroller.Trigger.alpha = 1; })
			BackpackScroller.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.BackpackScrollerIsDragging==true) return; me.BackpackScroller.Trigger.alpha = 0.5; })
			BackpackScroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OVER, function() {me.BackpackScroller.ArrowUp.alpha = 1;})
			BackpackScroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OUT, function() {me.BackpackScroller.ArrowUp.alpha = 0.5;})
			BackpackScroller.ArrowUp.addEventListener(MouseEvent.CLICK, function() {me.handleBackpackScrollerUpDown(-10);})
			BackpackScroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OVER, function() {me.BackpackScroller.ArrowDown.alpha = 1;})
			BackpackScroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OUT, function() {me.BackpackScroller.ArrowDown.alpha = 0.5; })
			BackpackScroller.ArrowDown.addEventListener(MouseEvent.CLICK, function() {me.handleBackpackScrollerUpDown(+10);})
			
			BackpackScroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, backpackScrollerHandler);
			function backpackScrollerHandler(e:MouseEvent):void{
				if(e.buttonDown){
					var startY = me.BackpackScroller.Field.y;
					var endY = me.BackpackScroller.Field.height - me.BackpackScroller.Trigger.height;
					me.BackpackScrollerIsDragging = true;
					me.BackpackScroller.Trigger.startDrag(false, new Rectangle (me.BackpackScroller.Trigger.x, startY, 0, endY));
				}
			}

			this.Title1.text = "$FR_SAFELOCK_TITLE";
			this.Equiped.text = "$FR_INVENTORY_EQUIPED";
			this.QuickSlots.text = "$FR_INVENTORY_QUICKSLOTS";
			
			this.setChildIndex(this.MsgBox, this.numChildren-1);
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			var	dist:Number;
			var	h;
			
			if (e.stageX > this.Mask.x && 
				e.stageX < this.Mask.x + this.Mask.width + 45 &&
				e.stageY > this.Mask.y && 
				e.stageY < this.Mask.y + this.Mask.height)
			{
				dist = (inventorySlots.height - this.Mask.height) / 99;
				h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
			else if (e.stageX > this.BackpackMask.x && 
				e.stageX < this.BackpackMask.x + this.BackpackMask.width + 45 &&
				e.stageY > this.BackpackMask.y && 
				e.stageY < this.BackpackMask.y + this.BackpackMask.height)
			{
				dist = (backpackSlotMasks.height - this.BackpackMask.height) / 99;
				h = BackpackScroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleBackpackScrollerUpDown(-dist);
				else
					handleBackpackScrollerUpDown(dist);
			}
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
				var endY = Scroller.Field.height - Scroller.Trigger.height+25;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}

		public function handleBackpackScrollerUpDown(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = BackpackScroller.Field.y; 
				if(BackpackScroller.Trigger.y > startY) { 
					BackpackScroller.Trigger.y += delta; 
					if(BackpackScroller.Trigger.y < startY) 
						BackpackScroller.Trigger.y = startY; 
				} 
			}
			else
			{
				var endY = BackpackScroller.Field.height - BackpackScroller.Trigger.height+25;
				if(BackpackScroller.Trigger.y < endY) { 
					BackpackScroller.Trigger.y += delta; 
					if(BackpackScroller.Trigger.y > endY) 
						BackpackScroller.Trigger.y = endY; 
				} 
			}
		}

		// event functions
		public	function ButtonRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function ButtonRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function ButtonPressFn(evt:Event)
		{
			if (evt is MouseEventEx)
			{
				if ((evt as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					return;
				}
			}
			
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");

				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as Inventory))
				{
					p = p.parent as MovieClip;
				}
				
				(p as Inventory).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function Activate()
		{
			this.mouseEnabled = true;
			
			var	survivor:Survivor = api.survivor;
			this.DescrName.text = "";
			this.DescrText.text = "";
			this.DescrText2.text = "";
			this.Scroller.visible = false;

			fillInventory (0);
			showDescription (null);
			showBackpack ();
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, endDrag);
			stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyBoardDown);
			stage.addEventListener(KeyboardEvent.KEY_UP, onKeyBoardUp);
			isShiftKeyDown = false;
		}
		
		public function Deactivate()
		{
			this.mouseEnabled = false;

			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, endDrag);
			stage.removeEventListener(KeyboardEvent.KEY_DOWN, onKeyBoardDown);
			stage.removeEventListener(KeyboardEvent.KEY_UP, onKeyBoardUp);
		}

		function onKeyBoardUp(e:KeyboardEvent):void
		{
			isShiftKeyDown = false;
		}
		function onKeyBoardDown(e:KeyboardEvent):void
		{
			if ( e.shiftKey) //if shift
			{
				isShiftKeyDown = true;
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button.indexOf("Tab") != -1)
			{
				var	tab = int (button.slice (3)) - 1;

				fillInventory (tab);
			}
			else if (button == "BtnBack")
			{
				this.BtnBack.State = "off";
				this.BtnBack.gotoAndPlay("out");

				SafelockEvents.eventReturnToGame();
			}
			else if (button == "BtnChange")
			{
				this.BtnChange.State = "off";
				this.BtnChange.gotoAndPlay("out");

				SafelockEvents.eventChangeKeyCode();
			}
			else if (button == "BtnPickup")
			{
				this.BtnPickup.State = "off";
				
				SafelockEvents.eventPickupLockbox();
			}
		}
		
		public	function fillInventory (tabID:int)
		{
			var inventoryDB:Array = api.InventoryDB;
			var sortedItemDB:Array = new Array();
			var	invItem:InventoryItem;
			var	i;
			var	t;
			var	Name;
			var	item:Item;

			for(i = 1; i <= 20; ++i)
			{
				Name = "TableSlot"+i;
				this[Name].visible = false;
			}
			
			while(inventorySlots.numChildren>0)
				inventorySlots.removeChildAt(0);
			
			var	survivor:Survivor = api.survivor;
			if(survivor.globalInventory==false)
				return;
			
			for (i=0; i<inventoryDB.length; i++)
			{
				invItem = inventoryDB[i];				
				item = api.getItemByID(invItem.itemID);
				
				if(item )
				{
					sortedItemDB.push(inventoryDB[i]);
				}
			}
			
			if (sortedItemDB.length > 20)
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
				Scroller.visible = false;
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Tweener.removeTweens(inventorySlots);
			}

			var slotOffsetX:int = 979;
			var	slotOffsetY:int = 196.5;
			var	slotWidth:int = 200;
			var	slotHeight:int = 99;

			inventorySlots.y = 0;

			for (i = 0; i < sortedItemDB.length; i++)
			{
				var inventorySlot = null;
				
				if (inventorySlots.numChildren > i)
				{
					inventorySlot = inventorySlots.getChildAt(i);
				}
				else
				{
					inventorySlot = new warz.safelock.InvTableSlot();
					inventorySlot.name = "InventorySlot" + (i + 1);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, InventoryRollOverFn);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, InventoryRollOutFn);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startInventoryDrag);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endInventoryDrag);
					inventorySlots.addChild(inventorySlot);
				}

				inventorySlot.Focus.visible = false;
				invItem = sortedItemDB[i];
				item = api.getItemByID(invItem.itemID);

				inventorySlot.x = slotOffsetX + (i % 4) * slotWidth;
				inventorySlot.y = slotOffsetY + int (i / 4) * slotHeight;

				if (inventorySlot.currentFrame != 0)
					inventorySlot.gotoAndStop(0);
				
				if (!inventorySlot.Item ||
					inventorySlot.Item.itemID != item.itemID)
				{
					if (inventorySlot.Pic.numChildren > 0)
						inventorySlot.Pic.removeChildAt(0);
	
					loadSlotIcon (item.Icon, inventorySlot.Pic, 1);
					
					inventorySlot.Pic.visible = true;
					
					if (inventorySlot.PicFX)
					{
						if (inventorySlot.PicFX.numChildren > 0)
							inventorySlot.PicFX.removeChildAt(0);
		
						loadSlotIcon (item.Icon, inventorySlot.PicFX, 1);
						
						inventorySlot.PicFX.visible = false;
					}
				}
				
				inventorySlot.Num.Text.text = invItem.quantity;
				inventorySlot.Name.Text.text = item.Name;
				
				inventorySlot.Item = item;
				inventorySlot.InventoryItem = invItem;
				inventorySlot.Type = "taken";
			}
			
			for (;i < 20; i++)
			{
				inventorySlot = null;
				
				if (inventorySlots.numChildren > i)
				{
					inventorySlot = inventorySlots.getChildAt(i);
				}
				else
				{
					inventorySlot = new warz.safelock.InvTableSlot();
					inventorySlot.name = "InventorySlot" + (i + 1);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, InventoryRollOverFn);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, InventoryRollOutFn);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startInventoryDrag);
					inventorySlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endInventoryDrag);
					inventorySlots.addChild(inventorySlot);
				}
				
				if (inventorySlot.currentLabel != "available")
					inventorySlot.gotoAndPlay("available");
				
				if (inventorySlot.Pic &&
					inventorySlot.Pic.numChildren > 0)
					inventorySlot.Pic.removeChildAt(0);

				inventorySlot.name = "InventorySlot" + (i + 1);

				inventorySlot.x = slotOffsetX + (i % 4) * slotWidth;
				inventorySlot.y = slotOffsetY + int (i / 4) * slotHeight;
				inventorySlot.Type = "available";
				inventorySlot.Item = null;
				inventorySlot.Focus.visible = false;

				if (inventorySlot.Num)
					inventorySlot.Num.Text.text = "";
				if (inventorySlot.Name)
					inventorySlot.Name.Text.text = "";
			}
			
			inventorySlots.y = 0;
		}

		public	function InventoryRollOverFn(evt:Event)
		{
			if (ItemIsDragging || evt.currentTarget.parent.Item)
			{
				if (evt.currentTarget.parent.Type == "available")
				{
					if (evt.currentTarget.parent.Pic.numChildren > 0)
						evt.currentTarget.parent.Pic.removeChildAt(0);
	
					evt.currentTarget.parent.Num.Text.text = "";
					evt.currentTarget.parent.Name.Text.text = "";
				}
				
				if (evt.currentTarget.parent.Type != "dragged")
				{
					evt.currentTarget.parent.gotoAndPlay("over");
				}
			}
		}
		
		public	function InventoryRollOutFn(evt:Event)
		{
			if (ItemIsDragging || evt.currentTarget.parent.Item)
			{
				if(evt.currentTarget.parent.Type != "dragged")
				{
					evt.currentTarget.parent.gotoAndPlay("out");
				}
			}
		}

		public	function scrollItems(e:Event)
		{
			if (!Scroller || Scroller.visible == false)
			{
				inventorySlots.y = 0;
				return;
			}
			
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = inventorySlots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = this.Mask.height;
			h -= mh;
			h /= h1;
			
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 99);
			
			if (inventorySlots.y != 99 * step)
			{
				Tweener.addTween(inventorySlots, {y:99 * step, time:api.tweenDelay, transition:"linear"});
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
				slotWidth = 200;
				slotHeight = 95;
				slotX = 110;
				slotY = 70;
			}
			else if (dat.slotType == 2)
			{
				slotWidth = 200;
				slotHeight = 100;
				slotX = 0;
				slotY = 0
			}
			else if (dat.slotType == 3)
			{
				slotWidth = 200;
				slotHeight = 99;
				slotX = 110;
				slotY = 55;
			}
			else if (dat.slotType == 4)
			{
				slotWidth = 400;
				slotHeight = 220;
				slotX = 215;
				slotY = 150;
			}
			else if (dat.slotType == 5)
			{
				slotWidth = 256;
				slotHeight = 140;
				slotX = 128;
				slotY = 80;
			}
			else if (dat.slotType == 6)
			{
				slotWidth = 205;
				slotHeight = 131;
				slotX = 110;
				slotY = 70;
			}

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x += slotX; 
			bitmap.y += slotY; 
		
			dat.imageHolder.scaleX = dat.imageHolder.scaleY = 1;
			dat.imageHolder.addChild (bitmap);
		}
		
		var	descriptionItem;
		var	nextDescriptionItem = null;
		
		public	function showDescription (item = null)
		{
			if (item == null)
			{
				item = nextDescriptionItem;
				nextDescriptionItem = null;
			}
			
			if (descriptionItem)
			{
				if (descriptionItem.Focus)
					descriptionItem.Focus.visible = false;
			}
			
			var	survivor:Survivor = api.survivor;
			if(!survivor.globalInventory)
				item = null;
			
			if (item && item.Item)
			{
				this.DescrName.text = item.Item.Name;
				this.DescrText.text = item.Item.desc;
				
				if (item.InventoryItem)
					this.DescrText2.text = item.InventoryItem.Description;
				else
					this.DescrText2.text = item.BackpackItem.Description;
				
				if (this.DescrPic.numChildren > 0)
					this.DescrPic.removeChildAt(0);
					
				loadSlotIcon  (item.Item.Icon, this.DescrPic, 4);
				
				if (item.Focus)
					item.Focus.visible = true;
			}
			else
			{
				if (this.DescrPic.numChildren > 0)
					this.DescrPic.removeChildAt(0);

				this.DescrName.text = "";
				this.DescrText.text = "";
				this.DescrText2.text = "";
			}
			
			descriptionItem = item;
		}

		public	function startInventoryDrag(e:MouseEvent) 
		{
			if (e.currentTarget.parent.Item == null)
				return;
				
			if (e is MouseEventEx)
			{
				if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					if (DraggedItem == null)
					{
						var	inventoryItem:InventoryItem = e.currentTarget.parent.InventoryItem;
						
						if (inventoryItem)
						{
							SoundEvents.eventSoundPlay("menu_click");
							if(isShiftKeyDown && inventoryItem.quantity>1)
							{
								MoveItem_inventoryID = inventoryItem.inventoryID;
								MoveItem_slot = -1;
								MoveItem_Quantity = inventoryItem.quantity;
								api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackFromInventoryCallback);
							}
							else
								SafelockEvents.eventBackpackFromInventory (inventoryItem.inventoryID, -1, 1);
						}
					}
						
					return;
				}
			}
			
			DraggedItem = e.currentTarget.parent;
			ItemIsDragging = true;
			
			SoundEvents.eventSoundPlay("menu_click");
			
			if (DraggedItem.currentLable != "pressed")
				DraggedItem.gotoAndPlay ("pressed");

			if (DragMovie.numChildren > 0)
				DragMovie.removeChildAt(0);
			
			var	item:Item = DraggedItem.Item;
			
			if (item)
			{
				loadSlotIcon (item.Icon, DragMovie, 2);
			}
			
			if (DraggedItem.PicFX)
			{
				DraggedItem.PicFX.visible = true;
				DraggedItem.Pic.visible = false;
			}

			DraggedItem.OldType = DraggedItem.Type;
			DraggedItem.Type = "dragged";
			
			DragMovie.visible = true;
			DragMovie.x = e.stageX;
			DragMovie.y = e.stageY;
			DragMovie.startDrag(false);
			DragMovie.mouseEnabled = false;
			
			showDescription (DraggedItem);
		}
		
		public function endInventoryDrag(e:MouseEvent) 
		{ 
			if (e is MouseEventEx)
			{
				if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					return;
				}
			}
			
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 

			BackpackScrollerIsDragging = false;
			BackpackScroller.Trigger.stopDrag(); 

		
			if (e.currentTarget.parent.Type == "available")
			{
				if (e.currentTarget.parent.currentLabel != "available")
					e.currentTarget.parent.gotoAndPlay("available");
			}
			else
			{
				if (e.currentTarget.parent.currentLable != "out")
					e.currentTarget.parent.gotoAndPlay("out");
			}

			if (ItemIsDragging)
			{
				if (e.currentTarget.parent.name.indexOf("InventorySlot") != -1)
				{
					var	backpack:Object = e.currentTarget.parent;
					var	slot:int = backpack.name.slice (12) - 1;
					var	item:Item = DraggedItem.Item;
					var	backpackItem:BackpackItem = DraggedItem.BackpackItem;
				
					if (DraggedItem.PicFX)
					{
						DraggedItem.PicFX.visible = false;
					}
					
					DraggedItem.Pic.visible = true;

// move item to this backpack slot					
					if (DraggedItem is warz.safelock.InvBackpackHotSlot ||
						DraggedItem is warz.safelock.InvBackpackSlot ||
						DraggedItem is warz.safelock.InvMainSlotLong ||
						DraggedItem is warz.safelock.InvMainSlotShort)
					{
						if (api.isDebug)
						{
							var survivor:Survivor = api.survivor;
							var	inventoryItem:InventoryItem = api.getInventorySlot (slot);
							
							if (inventoryItem && inventoryItem.itemID == item.itemID)
							{
								inventoryItem.quantity += 1;
							}
							else
							{
								inventoryItem = api.getInventoryItemByID(item.itemID);
								
								if (inventoryItem)
									inventoryItem.quantity += 1;
								else
								{
									api.addInventoryItem(int (Math.random() * 100000) + 500000, item.itemID, 1, -1, 0, false);
								}
							}
							
							backpackItem.quantity -= 1;
							
							if (backpackItem.quantity == 0)
							{
								survivor.removeBackpackItem (backpackItem.slotID);
							}
							
							api.backpackToInventorySuccess();
						}
						else 
						{
							if(isShiftKeyDown && backpackItem.quantity>1)
							{
								MoveItem_slot = backpackItem.slotID;
								MoveItem_Quantity = backpackItem.quantity;
								api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackToInventoryCallback);
							}
							else
								SafelockEvents.eventBackpackToInventory (backpackItem.slotID, backpackItem.quantity);
						}
						
						DraggedItem.Type = "available";
					}
					else
					{
						DraggedItem.Type = DraggedItem.OldType;
						
						if (DraggedItem.currentLabel != "out")
						{
							DraggedItem.gotoAndPlay ("out");
						}															  
					}
				}
				
				SoundEvents.eventSoundPlay("menu_use");
				
				DraggedItem = null; 
				ItemIsDragging = false;
				DragMovie.visible = false;
				DragMovie.stopDrag(); 
			}
		}
		
		public	function startBackpackDrag(e:MouseEvent) 
		{
			if (!e.currentTarget.parent.Item)
				return;
			
			if (e is MouseEventEx)
			{
				if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					if (DraggedItem == null)
					{
						var	backpackItem:BackpackItem = e.currentTarget.parent.BackpackItem;
						
						if (backpackItem)
						{
							SoundEvents.eventSoundPlay("menu_click");
							if(isShiftKeyDown && backpackItem.quantity>1)
							{
								MoveItem_slot = backpackItem.slotID;
								MoveItem_Quantity = backpackItem.quantity;
								api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackToInventoryCallback);
							}
							else
								SafelockEvents.eventBackpackToInventory (backpackItem.slotID, 1);
						}					
					}

					return;
				}
			}

			ItemIsDragging = true;
			DraggedItem = e.currentTarget.parent;
			
			if (DraggedItem.currentLabel != "pressed")
				DraggedItem.gotoAndPlay ("pressed");
			
			if (DragMovie.numChildren > 0)
				DragMovie.removeChildAt(0);
			
			var	item:Item = DraggedItem.Item;
			
			if (item)
				loadSlotIcon (item.Icon, DragMovie, 2);

			if (DraggedItem.PicFX)
			{
				DraggedItem.PicFX.visible = true;
			}
			
			SoundEvents.eventSoundPlay("menu_click");
			
			DraggedItem.Pic.visible = false;
			DraggedItem.OldType = DraggedItem.Type;
			DraggedItem.Type = "dragged";
			
			DragMovie.visible = true;
			DragMovie.x = e.stageX;
			DragMovie.y = e.stageY;
			DragMovie.startDrag(false);
			DragMovie.mouseEnabled = false;

			showDescription (DraggedItem);
		}

		public function endBackpackDrag(e:MouseEvent) 
		{ 
			if (e is MouseEventEx)
			{
				if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					return;
				}
			}

			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 

			BackpackScrollerIsDragging = false;
			BackpackScroller.Trigger.stopDrag(); 

			if (ItemIsDragging)
			{
				if (e.currentTarget.parent.name.indexOf("BackpackSlot") != -1)
				{
					var	backpack:Object = e.currentTarget.parent;
					var	slot:int = backpack.name.slice (12) - 1;
					var	item:Item = DraggedItem.Item;
					var	backpackItem:BackpackItem;
					var survivor:Survivor = api.survivor;
				
					var	isOK:Boolean = false;
						
					if (DraggedItem.PicFX)
					{
						DraggedItem.PicFX.visible = false;
					}
					
					DraggedItem.Pic.visible = true;
					
					isOK = isItemAllowedInSlot (item.catID, slot);
					
// move item to this backpack slot					
					if (isOK &&
						DraggedItem is warz.safelock.InvTableSlot)
					{
						var	inventoryItem:InventoryItem = DraggedItem.InventoryItem;
						backpackItem = survivor.getBackpackItem (slot);
						
						if (backpack.Item)
						{
							isOK = isItemAllowedInSlot (backpack.Item.catID, backpackItem.slotID);
						}
						
						if (isOK)
						{
							nextDescriptionItem = backpack;
							
							if (api.isDebug)
							{
								var	ok:Boolean = false;
								
								if (backpackItem && 
									backpackItem.itemID == item.itemID &&
									backpackItem.var1 == -1 &&
									item.isStackable)
								{
									backpackItem.quantity += 1;
									ok = true;
								}
								else if (!backpackItem)
								{
									survivor.addBackpackItem (slot, inventoryItem.inventoryID, item.itemID, 1, -1, 0, inventoryItem.Description);
									ok = true;
								}
								
								if (ok)
								{
									inventoryItem.quantity -= 1;
									
									if (inventoryItem.quantity == 0)
									{
										api.removeInventoryItem (item.itemID);
									}
								}
								
								api.backpackFromInventorySuccess();
							}
							else 
							{
								if(isShiftKeyDown && inventoryItem.quantity>1)
								{
									MoveItem_inventoryID = inventoryItem.inventoryID;
									MoveItem_slot = slot;
									MoveItem_Quantity = inventoryItem.quantity;
									api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackFromInventoryCallback);
								}
								else
									SafelockEvents.eventBackpackFromInventory (inventoryItem.inventoryID, slot, inventoryItem.quantity);
							}
						}
					}
					else if (isOK &&
							 (DraggedItem is warz.safelock.InvBackpackHotSlot ||
							 DraggedItem is warz.safelock.InvBackpackSlot ||
							 DraggedItem is warz.safelock.InvMainSlotLong ||
							 DraggedItem is warz.safelock.InvMainSlotShort))
					{
						backpackItem = DraggedItem.BackpackItem;
						
						isOK = true;
						
						if (backpack.Item)
						{
							isOK = isItemAllowedInSlot (backpack.Item.catID, backpackItem.slotID);
						}

						if (isOK)
						{
							nextDescriptionItem = backpack;
							if (api.isDebug)
							{
								survivor.swapBackpackSlots (backpackItem.slotID, slot);
	
								api.backpackGridSwapSuccess();
							}
							else
							{
								if (backpackItem && backpackItem.slotID != slot)
								{
									SafelockEvents.eventBackpackGridSwap (backpackItem.slotID, slot);
								}
							}
							
							if (backpack.currentLabel != "over")
								backpack.gotoAndPlay ("over");
						}
						else
						{
							if (DraggedItem.currentLabel != "out")
								DraggedItem.gotoAndPlay ("out");
						}
					}
					
					if (!isOK)
					{
						backpackItem = survivor.getBackpackItem (slot);
						
						if (DraggedItem.currentLabel != "out")
							DraggedItem.gotoAndPlay ("out");

						if (!backpackItem)
						{
							if (backpack.currentLabel != "available")
								backpack.gotoAndPlay ("available");

							if (slot == 1)
							{
								if (backpack.Backpic.currentLabel != "hg")
									backpack.Backpic.gotoAndPlay ("hg");
							}
							else if (slot == 6)
							{
								if (backpack.Backpic.currentLabel != "gear")
									backpack.Backpic.gotoAndPlay ("gear");
							}
							else if (slot == 7)
							{
								if (backpack.Backpic.currentLabel != "helmet")
									backpack.Backpic.gotoAndPlay ("helmet");
							}
							else if (slot != 0)
							{
								backpack.gotoAndPlay ("out");
							}
						}
					}				
				}

				SoundEvents.eventSoundPlay("menu_use");
				
				DraggedItem = null; 
				ItemIsDragging = false;
				DragMovie.visible = false;
				DragMovie.stopDrag(); 
			}
		}
		
		public	function eventBackpackFromInventoryCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				var q:uint = uint(text);
				if(q>0)
					SafelockEvents.eventBackpackFromInventory (MoveItem_inventoryID, MoveItem_slot, Math.min(q, MoveItem_Quantity));
			}
		}

		public	function eventBackpackToInventoryCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				var q:uint = uint(text);
				if(q>0)
					SafelockEvents.eventBackpackToInventory (MoveItem_slot, Math.min(MoveItem_Quantity, q));
			}
		}		
		
		public	function isItemAllowedInSlot (catID:int, slot:int):Boolean
		{
			var	itemSlots:Array= api.findItemSlot (catID);
			var	isOK:Boolean = false;
					
			switch (slot)
			{
				case	0:
					if (itemSlots[0] == 0 || (itemSlots.length>1 && itemSlots[1]==1))
					{
						isOK = true;
					}
					break;
					
				case	1:
					if (itemSlots[0] == 1 || (itemSlots.length>1 && itemSlots[1]==1))
					{
						isOK = true;
					}
					break;
					
				case	2:
				case	3:
				case	4:
				case	5:
					{
						isOK = true;
					}
					break;
					
				case	6:
					if (itemSlots[0] == 2)
						isOK = true;
					break;
					
				case	7:
					if (itemSlots[0] == 3)
						isOK = true;
					break;
					
				default:
					isOK = true;
					break;
			}
			
			return isOK;
		}
		
		public function endDrag(e:MouseEvent) 
		{ 
			if (e is MouseEventEx)
			{
				if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					return;
				}
			}
			
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 

			BackpackScrollerIsDragging = false;
			BackpackScroller.Trigger.stopDrag(); 
			
			if (ItemIsDragging)
			{
				if (DraggedItem)
				{
					if (DraggedItem.PicFX)
					{
						DraggedItem.PicFX.visible = false;
					}
					
					DraggedItem.Pic.visible = true;
					
					if (DraggedItem.currentLabel != "out")
						DraggedItem.gotoAndPlay ("out");
						
					DraggedItem.Type = "taken";
					DraggedItem = null;
				}

				ItemIsDragging = false;
				DragMovie.visible = false;
				DragMovie.stopDrag(); 
			}
		}
		
		public function showBackpack ()
		{
			for(var i = 1; i <= 24; ++i)
			{
				var Name = "BackpackSlot"+i;
				this[Name].visible = false;
			}
			
			var	survivor:Survivor = api.survivor;
			var slotOffsetX:int = 80;
			var	slotOffsetY:int = 168;
			var	slotWidth:int = 200;
			var	slotHeight:int = 99;

// backpack info
			this.Type.text = survivor.getBackpackName();
			this.MaxWeight.text = String (survivor.getBackpackMaxWeight ()) + " $FR_PAUSE_INVENTORY_LBS";
			this.Weight.text = survivor.weight.toFixed(1) + " $FR_PAUSE_INVENTORY_LBS";

			if (survivor.getBackpackSize () > 24)
			{
				BackpackScroller.visible = true;
				var me = this;
				
				BackpackScroller.Trigger.y = BackpackScroller.Field.y;
				BackpackScroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				
				BackpackScroller.ArrowDown.y = this.BackpackMask.height - 4;
				BackpackScroller.Field.height = (this.BackpackMask.height - (BackpackScroller.ArrowDown.height * 2));
			}
			else
			{
				BackpackScroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				BackpackScroller.visible = false;
				backpackSlotMasks.y = 0;
			}

			for (i = 0; i < survivor.getBackpackSize (); i++)
			{
				var backpackSlot = null;
				var	isNew:Boolean = false;
				
				if (i < 8)
				{
					if (backpackSlots.numChildren > i)
						backpackSlot = backpackSlots.getChildAt(i);
				}
				else
				{
					if (backpackSlotMasks.numChildren > i - 8)
						backpackSlot = backpackSlotMasks.getChildAt(i - 8);
				}

				if (i == 0)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.safelock.InvMainSlotLong();
						isNew = true;
					}

					backpackSlot.x = 85;
					backpackSlot.y = 158;
					backpackSlot.Key.Text.text = String (i + 1);

					if (backpackSlot.Btn.currentLabel != "long")
						backpackSlot.Btn.gotoAndPlay ("long");
				}
				else if (i == 1 || i == 6 || i == 7)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.safelock.InvMainSlotShort();
						isNew = true;
					}

					if (i == 1)
					{
						backpackSlot.x = 349;
					}
					else
					{
						backpackSlot.x = slotOffsetX + 467 + (i - 6) * 164;
					}

					backpackSlot.y = 158;
					
					if (i != 6 && i != 7)
						backpackSlot.Key.Text.text = String (i + 1);
					else
						backpackSlot.Key.visible = false;
					
					if (backpackSlot.Num)
						backpackSlot.Num.visible = false;
						
					if (backpackSlot.Btn.currentLabel != "short")
						backpackSlot.Btn.gotoAndPlay ("short");
				}
				else if (i < 8)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.safelock.InvBackpackHotSlot();
						isNew = true;
					}

					slotOffsetY = 245;
					backpackSlot.x = slotOffsetX + ((i - 2) % 4) * slotWidth;
					backpackSlot.y = slotOffsetY + slotHeight;
					
					if (backpackSlot.Key)
						backpackSlot.Key.Text.text = String (i + 1);
				}
				else 
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.safelock.InvBackpackSlot();
						isNew = true;
					}

					slotOffsetY = 543;
					backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
					backpackSlot.y = slotOffsetY + int ((i / 4) - 2) * slotHeight;
				}
				
				if (isNew)
				{
					Name = "BackpackSlot" + String(i + 1);
					
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					
					if (i >= 8)
						backpackSlotMasks.addChild(backpackSlot);
					else
						backpackSlots.addChild(backpackSlot);
				}
				
				if (backpackSlot.currentFrame != 0)
					backpackSlot.gotoAndStop (0);
						
				if (backpackSlot.Type == "off")
				{
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
				}

				var	backItem:BackpackItem = survivor.getBackpackItem(i);				

				if (backItem)
				{	
					var	item:Item = api.getItemByID (backItem.itemID);

					if (!backpackSlot.Item ||
						backpackSlot.Item.itemID != item.itemID)
					{
						if (backpackSlot.Pic)
						{
							if (backpackSlot.Pic.numChildren > 0)
								backpackSlot.Pic.removeChildAt(0);
								
							backpackSlot.Pic.visible = true;
							
							if (backpackSlot.slotID == 0)
								loadSlotIcon (item.Icon, backpackSlot.Pic, 5);
							else if (backpackSlot.slotID < 8)
								loadSlotIcon (item.Icon, backpackSlot.Pic, 1);
							else
								loadSlotIcon (item.Icon, backpackSlot.Pic, 3);
						}
						
						if (backpackSlot.PicFX)
						{
							if (backpackSlot.PicFX.numChildren > 0)
								backpackSlot.PicFX.removeChildAt(0);
								
							backpackSlot.PicFX.visible = false;
							
							if (backpackSlot.slotID == 0)
								loadSlotIcon (item.Icon, backpackSlot.PicFX, 5);
							else if (backpackSlot.slotID < 8)
								loadSlotIcon (item.Icon, backpackSlot.PicFX, 1);
							else
								loadSlotIcon (item.Icon, backpackSlot.PicFX, 3);
						}
					}

					if (backpackSlot.Num)
					{
						if (i == 0 || i == 1 || i == 6 || i == 7)
						{
							backpackSlot.Num.visible = false;
						}
						else
						{
							backpackSlot.Num.Text.text = backItem.quantity;
							backpackSlot.Num.visible = true;
						}
					}
						
					if (backpackSlot.Name)
					{
						backpackSlot.Name.visible = true;					
						backpackSlot.Name.Text.text = item.Name;
					}
					
					backpackSlot.Item = item;
					backpackSlot.BackpackItem = backItem;
					backpackSlot.Focus.visible = false;

					backpackSlot.Type = "taken";
				}
				else
				{
					backpackSlot.Item = null;
					backpackSlot.BackpackItem = null;
					
					if (backpackSlot.Num)
						backpackSlot.Num.visible = false;
					
					if (backpackSlot.Name)
						backpackSlot.Name.visible = false;
						
					if (backpackSlot.Pic)
						backpackSlot.Pic.visible = false;
					
					if (backpackSlot.PicFX)
						backpackSlot.PicFX.visible = false;

					if (backpackSlot.currentLabel != "available")
						backpackSlot.gotoAndPlay ("available");

					if (i == 1)
					{
						if (backpackSlot.Backpic.currentLabel != "hg")
							backpackSlot.Backpic.gotoAndPlay ("hg");
					}
					else
					{
						if (i == 6)
						{
							if (backpackSlot.Backpic.currentLabel != "gear")
								backpackSlot.Backpic.gotoAndPlay ("gear");
						}

						if (i == 7)
						{
							if (backpackSlot.Backpic.currentLabel != "helmet")
								backpackSlot.Backpic.gotoAndPlay ("helmet");
						}
					}

					backpackSlot.Focus.visible = false;
					backpackSlot.Type = "available";
				}
			}
			
			while(backpackSlotMasks.numChildren > (survivor.getBackpackSize() - 8))
				backpackSlotMasks.removeChildAt(survivor.getBackpackSize() - 8);
			
			for (; i < 24; i++)
			{
				backpackSlot = null;
				
				if (i < 8)
				{
					if (backpackSlots.numChildren > i)
						backpackSlot = backpackSlots.getChildAt(i);
				}
				else
				{
					if (backpackSlotMasks.numChildren > i - 8)
						backpackSlot = backpackSlotMasks.getChildAt(i - 8);
				}

				if (!backpackSlot)
				{
					backpackSlot = new warz.safelock.InvBackpackSlot();
					isNew = true;
				}

				slotOffsetY = 543;
				backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
				backpackSlot.y = slotOffsetY + int ((i / 4) - 2) * slotHeight;
				Name = "BackpackSlot" + String(i + 1);
				
				if (isNew)
				{
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					backpackSlotMasks.addChild(backpackSlot);
				}

				if (backpackSlot.Type != "off")
				{
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
				}
				
				backpackSlot.Type = "off";
				backpackSlot.Item = null;
				backpackSlot.BackpackItem = null;
				
				if (backpackSlot.currentLabel != "inactive")
					backpackSlot.gotoAndPlay ("inactive");
			}
		}
		
		public	function backpackSlotRollOverFn(evt:Event)
		{
			if (ItemIsDragging || evt.currentTarget.parent.Item)
			{
				if (evt.currentTarget.parent.Type != "dragged")
				{
					evt.currentTarget.parent.gotoAndPlay("over");
				}
				
				if (evt.currentTarget.parent.Type == "available")
				{
					if (evt.currentTarget.parent.Pic.numChildren > 0)
						evt.currentTarget.parent.Pic.removeChildAt(0);
	
					if (evt.currentTarget.parent.Num)
						evt.currentTarget.parent.Num.Text.text = "";
						
					if (evt.currentTarget.parent.Name)
						evt.currentTarget.parent.Name.Text.text = "";
				}
			}
		}
		
		public	function backpackSlotRollOutFn(evt:Event)
		{
			if (ItemIsDragging || evt.currentTarget.parent.Item)
			{
				if (evt.currentTarget.parent != DraggedItem)
				{
					if (evt.currentTarget.parent.Type == "available")
					{
						evt.currentTarget.parent.gotoAndPlay("available");
						
						switch (evt.currentTarget.parent.slotID)
						{
							case	1:
								if (evt.currentTarget.parent.Backpic.currentLabel != "hg")
									evt.currentTarget.parent.Backpic.gotoAndPlay ("hg");
								break;

							case	6:
								if (evt.currentTarget.parent.Backpic.currentLabel != "gear")
									evt.currentTarget.parent.Backpic.gotoAndPlay ("gear");
								break;
	
							case	7:
								if (evt.currentTarget.parent.Backpic.currentLabel != "helmet")
									evt.currentTarget.parent.Backpic.gotoAndPlay ("helmet");
								break;
						}						
					}
					else if (evt.currentTarget.parent.Type != "dragged")
					{
						evt.currentTarget.parent.gotoAndPlay("out");
					}
				}
			}
		}
		
		public	function scrollBackpackItems(e:Event)
		{
			var	a = BackpackScroller.Field.y;
			var	b = BackpackScroller.Trigger.y;
			var	dist = (BackpackScroller.Field.y - BackpackScroller.Trigger.y);
			var	h = backpackSlotMasks.height;
			var	h1 = BackpackScroller.Field.height - BackpackScroller.Trigger.height;
			var	mh = this.BackpackMask.height;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 99);
			
			if (backpackSlotMasks.y != 99 * step)
			{
				Tweener.addTween(backpackSlotMasks, {y:99 * step, time:api.tweenDelay, transition:"linear"});
			}
		}
	}
}
