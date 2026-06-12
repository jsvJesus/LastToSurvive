package warz.pause {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import warz.dataObjects.Survivor;
	import warz.dataObjects.BackpackItem;
	import warz.dataObjects.Item;
	import warz.dataObjects.InventoryItem;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.PauseEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import scaleform.gfx.MouseEventEx;
	import flash.events.KeyboardEvent;
	import caurina.transitions.Tweener;
	
	public class inventory extends MovieClip {
		
		public var api:warz.pause.pause=null;

		private	var	backpackSlots:MovieClip;
		private	var	backpackSlotMasks:MovieClip;
		
		public	var	ItemIsDragging:Boolean = false;
		public	var	DraggedItem:Object;
		public	var	DragMovie:MovieClip;
		
		private var BackpackScrollerIsDragging:Boolean;
		private	var	BackpackScroller:MovieClip;
		
		public	var	curSlot:int = -1;

		public function inventory() {
//			DropBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
//			DropBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
//			DropBtn.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);

//			ChangeBtn.Text.Text.text = "$FR_PAUSE_INVENTORY_CHANGE_BACKPACK";
//			DropBtn.Text.Text.text = "$FR_PAUSE_INVENTORY_DROP_ITEM";
			
			BtnChange.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			BtnChange.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			BtnChange.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			BtnChange.Text.Text.text = "$FR_PAUSE_INVENTORY_CHANGE_BACKPACK";
			
			BackpackType.text = "$FR_PAUSE_INVENTORY_BACKPACK_TYPE";
			MaxWeight.text = "$FR_PAUSE_INVENTORY_MAX_WEIGHT";
			CurrentWeight.text = "$FR_PAUSE_INVENTORY_WEIGHT";
			
			backpackSlots = new MovieClip ();
			Plate.addChild(backpackSlots);
			
			backpackSlotMasks = new MovieClip ();
			backpackSlotMasks.mask = Plate.BackpackMask;
			Plate.addChild(backpackSlotMasks);

			BackpackScroller = Plate.BackpackScroller;
			BackpackScrollerIsDragging = false;
			BackpackScroller.Field.alpha = 0.5;
			BackpackScroller.ArrowUp.alpha = 0.5;
			BackpackScroller.ArrowDown.alpha = 0.5;
			BackpackScroller.Trigger.alpha = 0.5;
			
			var	me = this;

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
			

			for (var a = 0; a < 6; a++)
			{
				var name = "Slot" + (a + 1);
				DialBox[name].Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
				DialBox[name].Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
				DialBox[name].Btn.addEventListener(MouseEvent.MOUSE_DOWN, DialPressFn);
			}
			
			DialBox["Slot1"].visible = false;
			DialBox["Slot2"].visible = false;
			DialBox["Slot3"].visible = false;
			DialBox["Slot4"].visible = false;
			DialBox["Slot5"].visible = false;
			DialBox["Slot6"].visible = false;
			
			DialBoxRepair.visible = false;
			DialBoxRepair.slotID = 0;
			DialBoxRepair.TitleDurCurr.text = DialBoxRepair.TitleDurCurrShad.text = "$FR_CurrentDurability";
			DialBoxRepair.Btn1.TitleDurNew.text = DialBoxRepair.Btn1.TitleDurNewShad.text = "$FR_NewDurability";
			DialBoxRepair.Btn2.TitleDurNew.text = DialBoxRepair.Btn2.TitleDurNewShad.text = "$FR_NewDurability";
			DialBoxRepair.Btn1.Pic.gotoAndStop(2);
			DialBoxRepair.Btn2.Pic.gotoAndStop(1);
			DialBoxRepair.Btn1.Premium.Text.text = DialBoxRepair.Btn1.Premium.TextShad.text = "$FR_RepairKit";
			DialBoxRepair.Btn2.Premium.Text.text = DialBoxRepair.Btn2.Premium.TextShad.text = "$FR_PremRepairKit";
			DialBoxRepair.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			DialBoxRepair.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			DialBoxRepair.Btn1.Btn.addEventListener(MouseEvent.MOUSE_DOWN, RepairPressFn);
			DialBoxRepair.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			DialBoxRepair.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			DialBoxRepair.Btn2.Btn.addEventListener(MouseEvent.MOUSE_DOWN, RepairPressFn);
			DialBoxRepair.X.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)	{
												evt.currentTarget.gotoAndPlay("over");});
			DialBoxRepair.X.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)	{
												evt.currentTarget.gotoAndPlay("out");});
			DialBoxRepair.X.addEventListener(MouseEvent.CLICK, function(evt:Event)	{
												DialBoxRepair.visible=false;});

			DragMovie = new MovieClip ();
			addChild(DragMovie);
		}
		
		public function showRepairMenu(slotID:uint, numRepairKits:uint, numPremium:uint, itemName:String, curDur:uint, newDur:uint, newPDur:uint)
		{
			DialBoxRepair.slotID = slotID;
			
			DialBoxRepair.TitleName.text = DialBoxRepair.TitleNameShad.text = itemName;
			DialBoxRepair.DurCurr.text = DialBoxRepair.DurCurrShad.text = curDur;
			if(numRepairKits>0)
				DialBoxRepair.Btn1.gotoAndStop(1);
			else
				DialBoxRepair.Btn1.gotoAndStop("inactive");
			if(numPremium>0)
				DialBoxRepair.Btn2.gotoAndStop(1);
			else
				DialBoxRepair.Btn2.gotoAndStop("inactive");
			
			if(numRepairKits>0)
			{
				DialBoxRepair.Btn1.DurNew.text = DialBoxRepair.Btn1.DurNewShad.text = newDur;
				DialBoxRepair.Btn1.TextNum.text = numRepairKits;
			}
			if(numPremium>0)
			{
				DialBoxRepair.Btn2.DurNew.text = DialBoxRepair.Btn2.DurNewShad.text = newPDur;
				DialBoxRepair.Btn2.TextNum.text = numPremium;
			}
			
			DialBoxRepair.x = DialBox.x;
			DialBoxRepair.y = DialBox.y;
			DialBoxRepair.visible = true;
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
				var endY = BackpackScroller.Field.height - BackpackScroller.Trigger.height+26;
				if(BackpackScroller.Trigger.y < endY) { 
					BackpackScroller.Trigger.y += delta; 
					if(BackpackScroller.Trigger.y > endY) 
						BackpackScroller.Trigger.y = endY; 
				} 
			}
		}
		
		public function Activate()
		{
			api = warz.pause.pause.api;

			this.mouseEnabled = true;
			this.mouseChildren = true;
			this.visible = true;
			showBackpack ();
			
			setCharacterName (api.survivor.Name);
			setReputationString(api.survivor.alignment);
			setPlayTime (api.survivor.timeplayed);
			setBackpackName (api.survivor.getBackpackName());
							 
			setMaxWeight (api.survivor.getBackpackMaxWeight ());
			setCurrentWeight (api.survivor.weight);
			
			stage.addEventListener(MouseEvent.MOUSE_UP, endDrag);
//			stage.addEventListener(KeyboardEvent.KEY_DOWN, keyDown);
//			stage.addEventListener(KeyboardEvent.KEY_UP, keyUp);
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);

			setCurSlot (-1);
			DialBox.visible = false;
			DialBoxRepair.visible = false;
			
			PauseEvents.eventDisableHotKeys(false);
			
			//showRepairMenu(0, 5, 0, "bla bla", 44, 55, 66);

		}
		
		public function Deactivate()
		{
			this.mouseEnabled = false;
			this.mouseChildren = false;
			this.visible = false;

			stage.removeEventListener(MouseEvent.MOUSE_UP, endDrag);
			removeEventListener(MouseEvent.MOUSE_DOWN, mouseDown);
//			stage.removeEventListener(KeyboardEvent.KEY_DOWN, keyDown);
//			stage.removeEventListener(KeyboardEvent.KEY_UP, keyUp);
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > Plate.BackpackMask.x && 
				e.stageX < Plate.BackpackMask.x + Plate.BackpackMask.width + 45 &&
				e.stageY > Plate.BackpackMask.y && 
				e.stageY < Plate.BackpackMask.y + Plate.BackpackMask.height)
			{
				var dist = (backpackSlotMasks.height - Plate.BackpackMask.height) / 98;
				var h = BackpackScroller.Field.height - BackpackScroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleBackpackScrollerUpDown(-dist);
				else
					handleBackpackScrollerUpDown(dist);
			}
		}

		public	function setCharacterName (name:String):void
		{
			CharName.text = name;
			CharNameShad.text = name;
		}
		
		public function setReputationString (rep:String):void
		{
			Reputation.text = rep;
			ReputationShad.text = rep;
		}
		
		public	function setPlayTime (timePlayed:int):void
		{
			var	seconds:int = timePlayed % 60;
			var	mins:int = (timePlayed / 60) % 60;
			var	hours:int = (timePlayed / 3600) % 24;
			var	days:int = (timePlayed / 86400);
			var	time:String = "";
			
			if (days > 1)
				time += String (days) + " $FR_Store_PluralDay ";
			else if (days == 1)
				time += String (days) + " $FR_Store_SingularDay ";
			if (hours > 0)
				time += String (hours) + " $FR_PAUSE_INVENTORY_HOURS ";
			else if (hours == 1)
				time += String (hours) + " $FR_PAUSE_INVENTORY_HOUR ";
			if (mins > 1)
				time += String (mins) + " $FR_PAUSE_INVENTORY_MINS";
			else if (mins == 1)
				time += String (mins) + " $FR_PAUSE_INVENTORY_MIN";

			CharPlaytime.text = "$FR_SURVIVOR_SCREEN_CREATE_SURVIVED "+time;				
			CharPlaytimeShad.text = "$FR_SURVIVOR_SCREEN_CREATE_SURVIVED "+time;
		}
		
		public	function setBackpackName (name:String):void
		{
			Type.text = name;
			TypeShad.text = name;
		}

		public	function setMaxWeight (weight:Number):void
		{
			Weight.text = String (weight) + " $FR_PAUSE_INVENTORY_LBS";
			WeightShad.text = String (weight) + " $FR_PAUSE_INVENTORY_LBS";
		}

		public	function setCurrentWeight (weight:Number):void
		{
			CurrWeight.text = weight.toFixed(1) + " $FR_PAUSE_INVENTORY_LBS";
			CurrWeightShad.text = weight.toFixed(1) + " $FR_PAUSE_INVENTORY_LBS";
		}
		
		public function showBackpack ()
		{
			if(api==null)
				return;
				
			for(var i = 1; i <= 28; ++i)
			{
				var Name = "Slot" + i;
				Plate[Name].visible = false;
			}
			
			var	survivor:Survivor = api.survivor;
			
			var slotOffsetX:int = 0;
			var	slotOffsetY:int = 330;
			var	slotWidth:int = 200;
			var	slotHeight:int = 98;

			i = 0;
			
			var backpackSlot = null;
			var	isNew:Boolean = false;
			
			if (survivor.getBackpackSize () > 28)
			{
				BackpackScroller.visible = true;
				var me = this;
				
				BackpackScroller.Trigger.y = BackpackScroller.Field.y;
				BackpackScroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				
				BackpackScroller.ArrowDown.y = Plate.BackpackMask.height - 4;
				BackpackScroller.Field.height = (Plate.BackpackMask.height - (BackpackScroller.ArrowDown.height * 2));
			}
			else
			{
				BackpackScroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				BackpackScroller.visible = false;
				backpackSlotMasks.y = 0;
			}
			
			setCurrentWeight (survivor.weight);

			for (; i < survivor.getBackpackSize (); i++)
			{
				isNew = false;
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

				if (i == 0)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.pause.InvMainSlotLong();
						isNew = true;
					}

					backpackSlot.Key.Text.text = String (i + 1);
					
					if (backpackSlot.Btn.currentLabel != "long")
						backpackSlot.Btn.gotoAndPlay ("long");
				}
				else if (i == 1 || i == 6 || i == 7)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.pause.InvMainSlotShort();
						isNew = true;
					}

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
						backpackSlot = new warz.pause.InvBackpackHotSlot();
						isNew = true;
					}

					if (backpackSlot.Key)
						backpackSlot.Key.Text.text = String (i + 1);
				}
				else 
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.pause.InvBackpackSlot();
						isNew = true;
					}
				}
				
				if (isNew)
				{
					Name = "Slot" + String(i + 1);
					
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					backpackSlot.State = "none";
					
					if (i < 8)
					{
						backpackSlots.addChild(backpackSlot);
						backpackSlot.x = Plate[Name].x;
						backpackSlot.y = Plate[Name].y;	
					}
					else
					{
						backpackSlotMasks.addChild(backpackSlot);
						backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
						backpackSlot.y = slotOffsetY + int ((i / 4) - 2) * slotHeight;
					}
				}

				if (backpackSlot.currentFrame != 0)
					backpackSlot.gotoAndStop (0);
					
				if (backpackSlot.State == "off")
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
						if (backpackSlot.Pic.numChildren > 0)
							backpackSlot.Pic.removeChildAt(0);
							
						if (backpackSlot.slotID == 0)
							loadSlotIcon (item.Icon, backpackSlot.Pic, 5);
						else
							loadSlotIcon (item.Icon, backpackSlot.Pic, 1);
							
						backpackSlot.Pic.visible = true;
						
						if (backpackSlot.PicFX)
						{
							if (backpackSlot.PicFX.numChildren > 0)
								backpackSlot.PicFX.removeChildAt(0);
								
							if (backpackSlot.slotID == 0)
								loadSlotIcon (item.Icon, backpackSlot.PicFX, 5);
							else
								loadSlotIcon (item.Icon, backpackSlot.PicFX, 1);

							backpackSlot.PicFX.visible = false;
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
						backpackSlot.Name.Text.text = item.Name;
						backpackSlot.Name.visible = true;
					}
					
					backpackSlot.Item = item;
					backpackSlot.BackpackItem = backItem;
					backpackSlot.Focus.visible = false;

					backpackSlot.State = "taken";
				}
				else
				{
					if (backpackSlot.currentLabel != "available")
						backpackSlot.gotoAndPlay ("available");
						
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
					backpackSlot.State = "available";
				}
			}

			while(backpackSlotMasks.numChildren > (survivor.getBackpackSize() - 8))
				backpackSlotMasks.removeChildAt(survivor.getBackpackSize() - 8);
			
			for (; i < 28; i++)
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
					backpackSlot = new warz.pause.InvBackpackSlot();
					isNew = true;
				}
				
				Name = "Slot" + String(i + 1);
				
				if (isNew)
				{
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					
					if (i < 8)
					{
						backpackSlots.addChild(backpackSlot);
						backpackSlot.x = Plate[Name].x;
						backpackSlot.y = Plate[Name].y;	
					}
					else
					{
						backpackSlotMasks.addChild(backpackSlot);
						backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
						backpackSlot.y = slotOffsetY + int ((i / 4) - 2) * slotHeight;
					}
				}

				if (backpackSlot.State != "off")
				{
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.removeEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
				}

				backpackSlot.State = "off";
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
				if (evt.currentTarget.parent.State != "dragged")
				{
					evt.currentTarget.parent.gotoAndPlay("over");
				}
				
				if (evt.currentTarget.parent.State == "available")
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
				if (evt.currentTarget.parent.State == "available")
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
				else if (evt.currentTarget.parent.State != "dragged")
				{
					evt.currentTarget.parent.gotoAndPlay("out");
				}
			}
		}
		
		public	function startBackpackDrag(e:MouseEvent) 
		{
			if(!ItemIsDragging){
				DraggedItem = e.currentTarget.parent;
				
				if (!DraggedItem.Item)
					return;
					
				DraggedItem.Focus.visible = false;
				setCurSlot (DraggedItem.slotID);
	
				if (e is MouseEventEx)
				{
					if ((e as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
					{
						showDialBox (true);
						return;
					}
				}
	
				DialBox.visible = false;
				DialBoxRepair.visible = false;
	
				ItemIsDragging = true;
				
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
				
				DraggedItem.Pic.visible = false;
				DraggedItem.OldState = DraggedItem.State;
				DraggedItem.State = "dragged";
	
				DragMovie.visible = true;
				DragMovie.x = e.stageX;
				DragMovie.y = e.stageY;
				DragMovie.startDrag(false);
				DragMovie.mouseEnabled = false;
				
				SoundEvents.eventSoundPlay("menu_click");
			}
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
			
			if (ItemIsDragging)
			{
				SoundEvents.eventSoundPlay("menu_use");
				if (e.currentTarget.parent.name.indexOf("Slot") != -1)
				{
					var	backpack:Object = e.currentTarget.parent;
					var	slot:int = backpack.name.slice (4) - 1;
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
							 (DraggedItem is warz.pause.InvBackpackHotSlot ||
							 DraggedItem is warz.pause.InvBackpackSlot ||
							 DraggedItem is warz.pause.InvMainSlotLong ||
							 DraggedItem is warz.pause.InvMainSlotShort))
					{
						backpackItem = DraggedItem.BackpackItem;
						
						if (backpack.Item)
						{
							isOK = isItemAllowedInSlot (backpack.Item.catID, backpackItem.slotID);
						}

						if (isOK && backpackItem && backpackItem.slotID != slot)
						{
							setCurSlot(slot, false);
							
							if (api.isDebug)
							{
								survivor.swapBackpackSlots (backpackItem.slotID, slot);
	
								api.backpackGridSwapSuccess ();
							}
							else
							{
								PauseEvents.eventBackpackGridSwap (backpackItem.slotID, slot);
							}
						}
						else
						{
							DraggedItem.Focus.visible = true;
							
							if (DraggedItem.currentLabel != "out")
								DraggedItem.gotoAndPlay ("out");
								
							DraggedItem.Type = DraggedItem.OldType;
						}
					}
					else 
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

						backpack.Focus.visible = false;
					}									
				}
			}
			
			DraggedItem = null; 
			ItemIsDragging = false;
			DragMovie.visible = false;
			DragMovie.stopDrag();
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
					if (itemSlots[0] == 1|| (itemSlots.length>1 && itemSlots[1]==1))
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

		public	function showCurItem ():void
		{
			if (curSlot != -1)
			{
				var	slot;
				
				if (curSlot < 8)
				{
					if (backpackSlots.numChildren > curSlot)
						slot = backpackSlots.getChildAt(curSlot);
				}
				else
				{
					if (backpackSlotMasks.numChildren > curSlot - 8)
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
				}
				
				if (slot)
				{
					if (slot.currentLable != "over")
						slot.gotoAndPlay("over");
						
					slot.Focus.visible = true;
					slot.State = "active";
				}
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
				slotY = 0;
			}
			else if (dat.slotType == 3)
			{
				slotWidth = 340;
				slotHeight = 180;
				slotX = 217;
				slotY = 225;
			}
			else if (dat.slotType == 5)
			{
				slotWidth = 256;
				slotHeight = 164;
				slotX = 128;
				slotY = 90;
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
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.State = "active";
				
				if (evt.currentTarget.parent.currentLabel != "out")
					evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as inventory))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as inventory).ActionFunction(evt.currentTarget.parent.name);
				
				setCurSlot (-1);
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "BtnChange")
			{
				BtnChange.State = "off";
				BtnChange.gotoAndPlay("out");

				api.Main.showChangeBackpack();
			}
			/*else if (button == "DropBtn")
			{
//				DropBtn.gotoAndPlay("out");
//				DropBtn.State = "off";

				if (curSlot != -1)
				{
					var	slot = backpackSlots.getChildAt(curSlot);
					
					if (slot.currentLable != "out")
						slot.gotoAndPlay("out");
					slot.Focus.visible = false;
					slot.State = "off";
					
					var	slotID = curSlot;
					
					setCurSlot(-1);

					if (api.isDebug)
					{
						api.survivor.removeBackpackItem(slotID);
						showBackpack ();
					}
					else
					{
						PauseEvents.eventBackpackDrop (slotID);
					}
				}
				else
				{
					api.showInfoMsg ("$FR_PAUSE_INVENTORY_DROP_ERROR", true, "$FR_PAUSE_INVENTORY_DROP_ITEM");
				}
			}*/
		}		
		
		public	function mouseDown (e:MouseEvent)
		{
			if (DialBox.visible)
			{
				DialBox.visible = false;
				DraggedItem = null;
			}
			if(DialBoxRepair.visible)
				DialBoxRepair.visible = false;

			removeEventListener(MouseEvent.MOUSE_DOWN, mouseDown);
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
		
		public	function scrollBackpackItems(e:Event)
		{
			var	a = BackpackScroller.Field.y;
			var	b = BackpackScroller.Trigger.y;
			var	dist = (BackpackScroller.Field.y - BackpackScroller.Trigger.y);
			var	h = backpackSlotMasks.height;
			var	h1 = BackpackScroller.Field.height - BackpackScroller.Trigger.height;
			var	mh = Plate.BackpackMask.height;
			h -= mh;
			h /= h1;
			
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 90);
			
			if (backpackSlotMasks.y != 98 * step)
			{
				Tweener.addTween(backpackSlotMasks, {y:98 * step, time:0.25, transition:"linear"});
			}

//			backpackSlotMasks.y = dist * h;
		}
		
		public	function setCurSlot (slot:int, doDescription:Boolean = true):void
		{
			var	s:Object;
			var	a:int;

			for (a = 0; a < backpackSlots.numChildren + backpackSlotMasks.numChildren; a++)
			{
				if (a < 8)
				{
					if (backpackSlots.numChildren > a)
						s = backpackSlots.getChildAt(a);
				}
				else
				{
					if (backpackSlotMasks.numChildren > a - 8)
						s = backpackSlotMasks.getChildAt(a - 8);
				}
				
				if (s && s.Focus)
					s.Focus.visible = false;
			}
			
			curSlot = slot;

			if (curSlot != -1)
			{
				if (curSlot < 8)
				{
					if (backpackSlots.numChildren > curSlot)
						s = backpackSlots.getChildAt(curSlot);
				}
				else
				{
					if (backpackSlotMasks.numChildren > curSlot - 8)
						s = backpackSlotMasks.getChildAt(curSlot - 8);
				}
				
				if (s && s.Focus)
					s.Focus.visible = true;
			}

			if (doDescription)
				showDescription ();
		}
		
		public	function showDescription ():void
		{
			var	slot = null;

			if (curSlot != -1)
			{
				if (curSlot < 8)
				{
					if (backpackSlots.numChildren > curSlot)
						slot = backpackSlots.getChildAt(curSlot);
				}
				else
				{
					if (backpackSlotMasks.numChildren > curSlot - 8)
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
				}
			}
			
			if (slot && slot.Item)
			{
				while (DescrBlock.Pic.numChildren)
					DescrBlock.Pic.removeChildAt(0);
					
				loadSlotIcon (slot.Item.Icon, DescrBlock.Pic, 3);
				
				DescrBlock.Name.text = slot.Item.Name;
				DescrBlock.DescrText.text = slot.Item.desc;
				DescrBlock.MiscText.text = slot.BackpackItem.Description;
				DescrBlock.Pic.visible = true;
				
				slot.Focus.visible = true;
			}
			else
			{
				DescrBlock.Name.text = "";
				DescrBlock.DescrText.text = "";
				DescrBlock.MiscText.text = "";
				DescrBlock.Pic.visible = false;
			}
		}
		
		private var dialBox_currentSlot:int = 1;
		
		public function addContextMenuOption(name:String, actionID:int)
		{
			var n = "Slot"+dialBox_currentSlot;
			dialBox_currentSlot++;
			
			DialBox[n].visible = true;
			DialBox[n].Text.Text.text = name;
			DialBox[n].Text.TextShad.text = name;
			DialBox[n].ActionID = actionID;
		}
		
		public	function showDialBox (state:Boolean):void
		{
			DialBox.visible = state;
			
			for(var i=1; i<=6; ++i)
			{
				DialBox["Slot"+i].visible = false;
			}
			
			if (state)
			{
				DialBox.x = DraggedItem.x + (DraggedItem.width >> 1) + 100;
				DialBox.y = DraggedItem.y + 150;

				dialBox_currentSlot = 1;
				
				var	item:Item = DraggedItem.Item;

				PauseEvents.eventShowContextMenuCallback(item.itemID, DraggedItem.slotID);
				if(api.isDebug)
				{
					// hard coded for now
					/*if(item.catID==30)
					{
						DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_EAT";
						DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_EAT";
					}
					else if(item.catID==33)
					{
						DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_DRINK";
						DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_DRINK";
					}
					else if(item.catID==11||item.catID==13||(item.catID>=20&&item.catID<=27)||item.catID==29)
					{
						if(item.catID==27 && DraggedItem.slotID>=2 && DraggedItem.slotID<=5)
							DialBox["Slot1"].visible = false;
						else if(DraggedItem.slotID!=0 && DraggedItem.slotID!=1 && DraggedItem.slotID!=6 && DraggedItem.slotID!=7)
						{
							DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_EQUIP";
							DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_EQUIP";
						}
						else
							DialBox["Slot1"].visible = false;
					}
					else if(item.catID==19 || item.catID==419)
					{
						DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_ATTACH";
						DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_ATTACH";
					}
					else if(item.catID==12)
					{
						DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_CHANGEBP";
						DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_CHANGEBP";
					}
					else
					{
						DialBox["Slot1"].Text.Text.text = "$FR_PAUSE_USE_ITEM";
						DialBox["Slot1"].Text.TextShad.text = "$FR_PAUSE_USE_ITEM";
					}*/
				
					addContextMenuOption("$FR_PAUSE_INVENTORY_DROP_ITEM", 20);
					if(item.catID >=20 && item.catID <=26)
					{
						addContextMenuOption("$FR_PAUSE_UNLOAD_WEAPON", 10);
					}
				}
				if(dialBox_currentSlot==1) // no options added
					DialBox.visible = false;
			}

			addEventListener(MouseEvent.MOUSE_DOWN, mouseDown, true);
		}
		
		public function RepairPressFn(e:Event):void
		{
			e.currentTarget.parent.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");
			if(e.currentTarget.parent.name=="Btn1") // regular repair
				PauseEvents.eventRepairItem(e.currentTarget.parent.parent.slotID, 0);
			else
				PauseEvents.eventRepairItem(e.currentTarget.parent.parent.slotID, 1);			
		}
		
		public	function DialPressFn (e:Event):void
		{
			var	slot;
					
			{
				if (curSlot != -1)
				{
					if (curSlot < 8)
					{
						slot = backpackSlots.getChildAt(curSlot);
					}
					else
					{
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
					}
					
					if (slot.currentLable != "out")
						slot.gotoAndPlay("out");
						
					slot.Focus.visible = false;
					slot.State = "off";

					var	dropSlot = curSlot;
					setCurSlot(-1);

					PauseEvents.eventContextMenu_Action (dropSlot, e.currentTarget.parent.ActionID);
				}
				else
				{
					api.showInfoMsg ("$FR_PAUSE_INVENTORY_CONTEXT_MENU_ERROR", true, "");
				}
			}
								
			/*if (e.currentTarget.parent.name == "Slot3")
			{
				if (curSlot != -1)
				{
					if (curSlot < 8)
					{
						slot = backpackSlots.getChildAt(curSlot);
					}
					else
					{
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
					}
					
					if (slot.currentLable != "out")
						slot.gotoAndPlay("out");
						
					slot.Focus.visible = false;
					slot.State = "off";

					PauseEvents.eventBackpackUnloadClip (curSlot);
				}
				else
				{
					api.showInfoMsg ("$FR_PAUSE_INVENTORY_DROP_ERROR", true, "$FR_PAUSE_INVENTORY_DROP_ITEM");
				}
			}
			if (e.currentTarget.parent.name == "Slot2")
			{
				if (curSlot != -1)
				{
					if (curSlot < 8)
					{
						slot = backpackSlots.getChildAt(curSlot);
					}
					else
					{
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
					}
					
					if (slot.currentLable != "out")
						slot.gotoAndPlay("out");
						
					slot.Focus.visible = false;
					slot.State = "off";

					var	dropSlot = curSlot;
					setCurSlot(-1);

					if (api.isDebug)
					{
						api.survivor.removeBackpackItem(dropSlot);
						showBackpack ();
					}
					else
					{
						PauseEvents.eventBackpackDrop (dropSlot);
					}
				}
				else
				{
					api.showInfoMsg ("$FR_PAUSE_INVENTORY_DROP_ERROR", true, "$FR_PAUSE_INVENTORY_DROP_ITEM");
				}
			}
			else if (e.currentTarget.parent.name == "Slot1")
			{
				if (curSlot != -1)
				{
					SoundEvents.eventSoundPlay("menu_use");
					
					if (curSlot < 8)
					{
						slot = backpackSlots.getChildAt(curSlot);
					}
					else
					{
						slot = backpackSlotMasks.getChildAt(curSlot - 8);
					}
					
					var	useSlot = curSlot;
					setCurSlot(-1);

					if (api.isDebug)
					{
						var item:BackpackItem = api.survivor.getBackpackItem (useSlot);
						
						item.quantity--;
						
						if (item.quantity <= 0)
							api.survivor.removeBackpackItem(useSlot);
							
						showBackpack ();
					}
					else
					{
						PauseEvents.eventBackpackUseItem (useSlot);
					}
				}
				else
				{
					api.showInfoMsg ("$FR_PAUSE_INVENTORY_DROP_ERROR", true, "$FR_PAUSE_INVENTORY_DROP_ITEM");
				}
			}*/
		}
	}
}
