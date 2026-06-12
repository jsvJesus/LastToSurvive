package warz.repair {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.RepairEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import scaleform.gfx.MouseEventEx;
	import caurina.transitions.Tweener;
	
	public class Inventory extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.repair.repair=null;
		
		private	var	backpackSlots:MovieClip;
		private	var	backpackSlotMasks:MovieClip;
		
		private var BackpackScrollerIsDragging:Boolean;
		public	var	BackpackScroller:MovieClip;

		public function Inventory() {
			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			this.BtnBack.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			this.BtnBack.Text.Text.text = "$HUD_Pause_ReturnToGame";

			this.BtnRepairAll.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			this.BtnRepairAll.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			this.BtnRepairAll.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			this.BtnRepairAll.Text.Text.text = "$FR_Repair_all_items_for";

			backpackSlots = new MovieClip ();
			this.addChild(backpackSlots);
			
			backpackSlotMasks = new MovieClip ();
			backpackSlotMasks.mask = this.BackpackMask;
			this.addChild(backpackSlotMasks);
			
			this.HeaderType.text = "$FR_PAUSE_INVENTORY_BACKPACK_TYPE";
			this.HeaderMaxWeight.text = "$FR_PAUSE_INVENTORY_MAX_WEIGHT";
			this.HeaderWeight.text = "$FR_PAUSE_INVENTORY_WEIGHT";
			
			var me = this;

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
			
			BackpackScroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startY = me.BackpackScroller.Field.y;
				var endY = me.BackpackScroller.Field.height - me.BackpackScroller.Trigger.height;
				me.BackpackScrollerIsDragging = true;
				me.BackpackScroller.Trigger.startDrag(false, new Rectangle (me.BackpackScroller.Trigger.x, startY, 0, endY));
			})

			this.Equiped.text = "$FR_INVENTORY_EQUIPED";
			this.QuickSlots.text = "$FR_INVENTORY_QUICKSLOTS";
			
			this.setChildIndex(this.MsgBox, this.numChildren-1);
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			var	dist:Number;
			var	h;
			
			if (e.stageX > this.BackpackMask.x && 
				e.stageX < this.BackpackMask.x + this.BackpackMask.width + 45 &&
				e.stageY > this.BackpackMask.y && 
				e.stageY < this.BackpackMask.y + this.BackpackMask.height)
			{
				dist = (backpackSlotMasks.height - this.BackpackMask.height) / 99;
				h = BackpackScroller.Field.height - BackpackScroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleBackpackScrollerUpDown(-dist);
				else
					handleBackpackScrollerUpDown(dist);
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
		
		public function setGD(gd:uint)
		{
			this.Money.GD.text = gd;
		}
		
		public function Activate()
		{
			this.mouseEnabled = true;
			
			showBackpack ();
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
		}
		
		public function Deactivate()
		{
			this.mouseEnabled = false;

			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
		}

		public function ActionFunction (button:String)
		{
			if (button == "BtnBack")
			{
				this.BtnBack.State = "off";
				this.BtnBack.gotoAndPlay("out");

				RepairEvents.eventReturnToGame();
			}
			else if (button == "BtnRepairAll")
			{
				this.BtnRepairAll.State = "off";
				this.BtnRepairAll.gotoAndPlay("out");

				RepairEvents.eventRepairAll();
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
		
		public function showBackpack ()
		{
			var totalPrice = 0;
			for(var i=0; i<api.BackpackRepairInfo.length; ++i)
				totalPrice += api.BackpackRepairInfo[i]["price"];
			this.BtnRepairAll.Text.Price.text = totalPrice;
			
			for(i = 1; i <= 24; ++i)
			{
				var Name = "BackpackSlot"+i;
				this[Name].visible = false;
			}
			
			var	survivor:Survivor = api.survivor;
			var slotOffsetX:int = 555;
			var	slotOffsetY:int = 182;
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
						backpackSlot = new warz.repair.InvMainSlotLong();
						isNew = true;
					}

					backpackSlot.x = 555;
					backpackSlot.y = 182;
					backpackSlot.Key.Text.text = String (i + 1);

					if (backpackSlot.Btn.currentLabel != "long")
						backpackSlot.Btn.gotoAndPlay ("long");
				}
				else if (i == 1 || i == 6 || i == 7)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.repair.InvMainSlotShort();
						isNew = true;
					}

					if (i == 1)
					{
						backpackSlot.x = 822;
					}
					else
					{
						backpackSlot.x = slotOffsetX + 467 + (i - 6) * 164;
					}

					backpackSlot.y = 182;
					
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
						backpackSlot = new warz.repair.InvBackpackHotSlot();
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
						backpackSlot = new warz.repair.InvBackpackSlot();
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
					backpackSlot.Btn.addEventListener(MouseEvent.CLICK, backpessPressFn);
					backpackSlot.Percents.text = "";
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
					backpackSlot.Btn.addEventListener(MouseEvent.CLICK, backpessPressFn);
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
					
					var repairInfo = api.getRepairInfo(i);
					if(repairInfo)
					{
						backpackSlot.Percents.text = repairInfo["itemCondition"]+"%";
						backpackSlot.RepairStatus.Titlle1.text = "$FR_ItemCondition";
						backpackSlot.RepairStatus.Titlle2.text = "$FR_ItemClickToRepair";
						backpackSlot.RepairStatus.TextGD.text = repairInfo["price"]+" GD";
						backpackSlot.repair = true;
						backpackSlot.gotoAndPlay("repair");
					}
					else
					{
						backpackSlot.Percents.text = "";
						backpackSlot.repair = false;
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
					backpackSlot = new warz.repair.InvBackpackSlot();
					isNew = true;
				}

				slotOffsetY = 543;
				backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
				backpackSlot.y = slotOffsetY + int ((i / 4) - 2) * slotHeight;
				Name = "BackpackSlot" + String(i + 1);
				
				if (isNew)
				{
					backpackSlot.name = Name;
					backpackSlot.Percents.text = "";
					backpackSlot.slotID = i;	
					backpackSlotMasks.addChild(backpackSlot);
				}

				if (backpackSlot.Type != "off")
				{
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.CLICK, backpessPressFn);
				}
				
				backpackSlot.Type = "off";
				backpackSlot.Item = null;
				backpackSlot.BackpackItem = null;
				
				if (backpackSlot.currentLabel != "inactive")
					backpackSlot.gotoAndPlay ("inactive");
			}
		}
		
		public function backpessPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.Item && evt.currentTarget.parent.repair == true)
			{
				RepairEvents.eventRepairItemInSlot(evt.currentTarget.parent.slotID);
			}
		}
		
		public	function backpackSlotRollOverFn(evt:Event)
		{
			if (evt.currentTarget.parent.Item)
			{
				if(evt.currentTarget.parent.repair == true)
					evt.currentTarget.parent.gotoAndPlay("repair_over");
				else
					evt.currentTarget.parent.gotoAndPlay("over");
				
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
			if (evt.currentTarget.parent.Item)
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
				else
				{
					if(evt.currentTarget.parent.repair == true)
						evt.currentTarget.parent.gotoAndPlay("repair_out");
					else
						evt.currentTarget.parent.gotoAndPlay("out");
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
