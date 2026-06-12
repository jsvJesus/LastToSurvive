package warz.trade {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.TradeEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import scaleform.gfx.MouseEventEx;
	import caurina.transitions.Tweener;
	import flashx.textLayout.operations.MoveChildrenOperation;
	import flash.geom.Point;
	
	public class TradeScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.trade.trade=null;
		
		public var Trade:MovieClip;
		public var MainMenu:MovieClip;
		
		private	var	backpackSlots:MovieClip;
		private	var	backpackSlotMasks:MovieClip;
		
		private var BackpackScrollerIsDragging:Boolean;
		public	var	BackpackScroller:MovieClip;

		public	var	ItemIsDragging:Boolean = false;
		public	var	DraggedItem:Object;
		public	var	DragMovie:MovieClip;

		private var isShiftKeyDown:Boolean = false;
		private var MoveItem_SrcSlotID;
		private var MoveItem_slot;
		private var MoveItem_Quantity;
		
		private var UserTradeDB:Array = new Array();
		private var OppositeTradeDB:Array = new Array();

		public function TradeScreen() {
			MainMenu.MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			MainMenu.MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			MainMenu.MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			MainMenu.MainMenu.MainMenuBtn6.TextShad.Text.text = MainMenu.MainMenu.MainMenuBtn6.Text.Text.text = "$HUD_Pause_ReturnToGame";

			backpackSlots = new MovieClip ();
			Trade.Plate.addChild(backpackSlots);
			
			backpackSlotMasks = new MovieClip ();
			backpackSlotMasks.mask = Trade.Plate.BackpackMask;
			Trade.Plate.addChild(backpackSlotMasks);
			
			Trade.SlotPopup.visible = false;
			Trade.CoverOpponent.mouseEnabled = false;
			
			DragMovie = new MovieClip ();
			Trade.addChild(DragMovie);
			
			Trade.BackpackType.text = "$FR_PAUSE_INVENTORY_BACKPACK_TYPE";
			Trade.MaxWeight.text = "$FR_PAUSE_INVENTORY_MAX_WEIGHT";
			Trade.CurrentWeight.text = "$FR_PAUSE_INVENTORY_WEIGHT";
			
			Trade.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Trade.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Trade.BtnDecline.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Trade.BtnDecline.Text.Text.text = "$FR_TRADE_DECLINE";

			Trade.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Trade.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Trade.BtnAccept.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Trade.BtnAccept.Text.Text.text = "$FR_TRADE_ACCEPT";
			
			Trade.UserTradeIndicator.Text.Text.text = "$FR_TRADE_ACCEPTED";
			Trade.OpponentTradeIndicator.Text.Text.text = "$FR_TRADE_ACCEPTED";

			var me = this;

			BackpackScrollerIsDragging = false;
			BackpackScroller = Trade.Plate.BackpackScroller;
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

			MainMenu.MainMenu.MainMenuBtn1.gotoAndPlay("pressed");
			MainMenu.MainMenu.MainMenuBtn1.TextShad.Text.text = MainMenu.MainMenu.MainMenuBtn1.Text.Text.text = "$FR_TRADE";
			
			this.setChildIndex(this.MsgBox, this.numChildren-1);
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			var	dist:Number;
			var	h;
			
			if (e.stageX > Trade.Plate.BackpackMask.x && 
				e.stageX < Trade.Plate.BackpackMask.x + Trade.Plate.BackpackMask.width + 45 &&
				e.stageY > Trade.Plate.BackpackMask.y && 
				e.stageY < Trade.Plate.BackpackMask.y + Trade.Plate.BackpackMask.height)
			{
				dist = (backpackSlotMasks.height - Trade.Plate.BackpackMask.height) / 99;
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
			if(evt.currentTarget.parent.currentFrameLabel != "inactive")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function ButtonRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentFrameLabel != "inactive")
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
			
			if(evt.currentTarget.parent.currentFrameLabel != "inactive")
			{
				SoundEvents.eventSoundPlay("menu_click");

				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as TradeScreen))
				{
					p = p.parent as MovieClip;
				}
				
				(p as TradeScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function addItemToUserTrade(slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, isConsumable:Boolean, description:String = "")
		{
			UserTradeDB[slotID] = new BackpackItem(slotID, inventoryID, itemID, quantity, var1, var2, isConsumable, description);
		}
		
		public function removeItemFromUserTrade(slotID:int)
		{
			UserTradeDB[slotID] = new BackpackItem(slotID, 0, 0, 0, 0, 0, false, "");
		}
		
		public function swapUserTradeItems(trgSlotID:int, srcSlotID:int)
		{
			var temp:BackpackItem = UserTradeDB[trgSlotID];
			UserTradeDB[trgSlotID] = UserTradeDB[srcSlotID];
			UserTradeDB[srcSlotID] = temp;

			// fix slotID now
			UserTradeDB[trgSlotID].slotID = trgSlotID;
			UserTradeDB[srcSlotID].slotID = srcSlotID;
		}
		
		public function addItemToOppositeTrade(slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, isConsumable:Boolean, description:String = "")
		{
			OppositeTradeDB[slotID] = new BackpackItem(slotID, inventoryID, itemID, quantity, var1, var2, isConsumable, description);
		}
		
		public function removeItemFromOppositeTrade(slotID:int)
		{
			OppositeTradeDB[slotID] = new BackpackItem(slotID, 0, 0, 0, 0, 0, false, "");
		}
		
		public function enableUserTradeIndicator(isSet:Boolean)
		{
			if(isSet)
				Trade.BtnAccept.gotoAndPlay(1);
			else
				Trade.BtnAccept.gotoAndStop("inactive");
		}
		
		public function setUserTradeIndicator(isSet:Boolean)
		{
			if(isSet)
				Trade.UserTradeIndicator.gotoAndStop("on");
			else
				Trade.UserTradeIndicator.gotoAndStop("off");
		}

		public function setOppositeTradeIndicator(isSet:Boolean)
		{
			if(isSet)
				Trade.OpponentTradeIndicator.gotoAndStop("on");
			else
				Trade.OpponentTradeIndicator.gotoAndStop("off");
		}
		
		public function setOppositePlayerName(n:String)
		{
			Trade.OpponentName.text = n;
		}
		
		public function clearTradeInfo()
		{
			UserTradeDB = new Array(); // reset
			for(var i=0; i<16; ++i)
				UserTradeDB.push(new BackpackItem(i, 0, 0, 0, 0, 0, false, ""));
			if(api.isDebug)
			{
				addItemToUserTrade(0, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToUserTrade(5, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToUserTrade(10, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToUserTrade(15, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
			}

			OppositeTradeDB = new Array(); // reset
			for(i=0; i<16; ++i)
				OppositeTradeDB.push(new BackpackItem(i, 0, 0, 0, 0, 0, false, ""));
			if(api.isDebug)
			{
				addItemToOppositeTrade(1, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToOppositeTrade(6, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToOppositeTrade(9, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
				addItemToOppositeTrade(14, 1000, 100003, 10, 20, 40, false, "dkjhdjfh");
			}						
		}

		public function Activate()
		{
			this.mouseEnabled = true;
			
			var	survivor:Survivor = api.survivor;
			
			Trade.UserTradeIndicator.gotoAndStop("off");
			Trade.OpponentTradeIndicator.gotoAndStop("off");

			if(api.isDebug)
			{
				clearTradeInfo();
				setOppositePlayerName("dsfjhd djfhdjfh");
			}

			showBackpack();
			showUserTrade();
			showOppositeTrade();
			
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
			if (button == "MainMenuBtn6")
			{
				MainMenu.MainMenu.MainMenuBtn6.State = "off";
				MainMenu.MainMenu.MainMenuBtn6.gotoAndPlay("out");

				TradeEvents.eventReturnToGame();
			}
			else if(button == "BtnAccept")
			{
				TradeEvents.eventTradeAccept();
				if(api.isDebug)
					setUserTradeIndicator(true);
			}
			else if(button == "BtnDecline")
			{
				TradeEvents.eventTradeDecline();
				if(api.isDebug)
					setUserTradeIndicator(false);
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
						
						var emptySlot = -1;
						for(var i=0; i<UserTradeDB.length; ++i)
							if(UserTradeDB[i].itemID==0)
							{
								emptySlot = i;
								break;
							}
						
						if (backpackItem && emptySlot !=-1)
						{
							SoundEvents.eventSoundPlay("menu_click");
							if(isShiftKeyDown && backpackItem.quantity>1)
							{
								MoveItem_SrcSlotID = backpackItem.slotID;
								MoveItem_slot = emptySlot;
								MoveItem_Quantity = backpackItem.quantity;
								api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackToTradeCallback);
							}
							else
								TradeEvents.eventBackpackToTrade(backpackItem.slotID, emptySlot, 1);
						}					
					}

					return;
				}
			}
			
			return; // drag and drop is disabled

			/*ItemIsDragging = true;
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
			DragMovie.mouseEnabled = false;*/
		}

		public	function startTradeDrag(e:MouseEvent) 
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
						
						var emptySlot = -1;
						for(var i=0; i<UserTradeDB.length; ++i)
							if(UserTradeDB[i].itemID==0)
							{
								emptySlot = i;
								break;
							}
						
						if (backpackItem && emptySlot !=-1)
						{
							SoundEvents.eventSoundPlay("menu_click");
							if(isShiftKeyDown && backpackItem.quantity>1)
							{
								MoveItem_SrcSlotID = backpackItem.slotID;
								MoveItem_slot = emptySlot;
								MoveItem_Quantity = backpackItem.quantity;
								api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventTradeToBackpackCallback);
							}
							else
								TradeEvents.eventTradeToBackpack(backpackItem.slotID, emptySlot, 1);
						}					
					}

					return;
				}
			}
			
			return; // drag and drop is disabled
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
			
			return; // drag & drop is disabled

			/*BackpackScrollerIsDragging = false;
			BackpackScroller.Trigger.stopDrag(); 

			if (ItemIsDragging)
			{
				if (e.currentTarget.parent.name.indexOf("Slot") != -1)
				{
					var	targetSlot:Object = e.currentTarget.parent;
					var	slot:int = 0;
					var	isOK:Boolean = false;
					var	item:Item = DraggedItem.Item;
					if(e.currentTarget.parent.name.indexOf("BackpackSlot")!=-1)
					{
						slot = targetSlot.name.slice (12) - 1;
						isOK = isItemAllowedInSlot (item.catID, slot);
					}
					else // TradeSlot
					{
						slot = targetSlot.name.slice (9) - 1;
						isOK = true; // will be allowed always
					}
						
					var	backpackItem:BackpackItem;
					var survivor:Survivor = api.survivor;
				
						
					if (DraggedItem.PicFX)
					{
						DraggedItem.PicFX.visible = false;
					}
					
					DraggedItem.Pic.visible = true;
					
					// move item to trade window
					if (isOK &&
						e.currentTarget.parent.name.indexOf("TradeSlot")!=-1)
					{
						var targetItem:BackpackItem = targetSlot.BackpackItem;
						var sourceItem:BackpackItem = DraggedItem.BackpackItem;
						
						var DraggedItemIsTradeItem = false;
						if(DraggedItem.name.indexOf("TradeSlot")!=-1)
							DraggedItemIsTradeItem = true;
							
						if(!DraggedItemIsTradeItem)
						{
							if (api.isDebug)
							{
								var	ok:Boolean = false;
								
								if (targetItem.itemID == sourceItem.itemID &&
									sourceItem.var1 == -1 &&
									item.isStackable)
								{
									targetItem.quantity += 1;
									ok = true;
								}
								else if (targetItem.itemID==0)
								{
									addItemToUserTrade(targetItem.slotID, 0, sourceItem.itemID, sourceItem.quantity, sourceItem.var1, sourceItem.var2, sourceItem.isConsumable, sourceItem.Description);
									ok = true;
								}
								
								if (ok)
								{
									survivor.removeBackpackItem(sourceItem.slotID);
								}
								
								api.updateBackpackAndTradeWindows();
							}
							else 
							{
								if(isShiftKeyDown && sourceItem.quantity>1)
								{
									MoveItem_SrcSlotID = sourceItem.slotID;
									MoveItem_slot = slot;
									MoveItem_Quantity = sourceItem.quantity;
									api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventBackpackToTradeCallback);
								}
								else
									TradeEvents.eventBackpackToTrade (sourceItem.slotID, slot, sourceItem.quantity);
							}
						}
						else if(targetItem.slotID != sourceItem.slotID) // swap trade slots
						{
							if (api.isDebug)
							{
								swapUserTradeItems(targetItem.slotID, sourceItem.slotID);
								api.updateBackpackAndTradeWindows();
							}
							else
							{
								TradeEvents.eventTradeGridSwap(targetItem.slotID, sourceItem.slotID);
							}
						}
					}
					// drag into backpack
					else if (isOK && e.currentTarget.parent.name.indexOf("BackpackSlot")!=-1)
					{
						var DraggedItemIsBackpackItem = false;
						if(DraggedItem.name.indexOf("BackpackSlot")!=-1)
							DraggedItemIsBackpackItem = true;
						
						backpackItem = DraggedItem.BackpackItem;
						
						isOK = true;
						if (targetSlot.Item)
						{
							isOK = isItemAllowedInSlot (targetSlot.Item.catID, backpackItem.slotID);
						}

						if (isOK)
						{
							if(DraggedItemIsBackpackItem)
							{
								if (api.isDebug)
								{
									survivor.swapBackpackSlots (backpackItem.slotID, slot);
		
									api.updateBackpackAndTradeWindows();
								}
								else
								{
									if (backpackItem && backpackItem.slotID != slot)
									{
										TradeEvents.eventBackpackGridSwap (backpackItem.slotID, slot);
									}
								}
							}
							else
							{
								targetItem = targetSlot.BackpackItem;
								sourceItem = DraggedItem.BackpackItem;
								// move item from trade window back into backpack
								if(api.isDebug)
								{
									ok = false;
											
									if (targetItem && targetItem.itemID == sourceItem.itemID &&
										sourceItem.var1 == -1 &&
										item.isStackable)
									{
										targetItem.quantity += 1;
										ok = true;
									}
									else if (targetItem==null)
									{
										survivor.addBackpackItem(slot, 0, sourceItem.itemID, sourceItem.quantity, sourceItem.var1, sourceItem.var2, sourceItem.Description);
										ok = true;
									}
										
									if (ok)
									{
										removeItemFromUserTrade(sourceItem.slotID);
									}
											
									api.updateBackpackAndTradeWindows();
								}
								else 
								{
									if(isShiftKeyDown && sourceItem.quantity>1)
									{
										MoveItem_SrcSlotID = sourceItem.slotID;
										MoveItem_slot = slot;
										MoveItem_Quantity = sourceItem.quantity;
										api.Main.MsgBox.showInfoInputMsg("$FR_HowManyItemsToMove", "", eventTradeToBackpackCallback);
									}
									else
										TradeEvents.eventTradeToBackpack(sourceItem.slotID, slot, sourceItem.quantity);
								}
							}
							
							if (targetSlot.currentLabel != "over")
								targetSlot.gotoAndPlay ("over");
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
							if (targetSlot.currentLabel != "available")
								targetSlot.gotoAndPlay ("available");

							if (slot == 1)
							{
								if (targetSlot.Backpic.currentLabel != "hg")
									targetSlot.Backpic.gotoAndPlay ("hg");
							}
							else if (slot == 6)
							{
								if (targetSlot.Backpic.currentLabel != "gear")
									targetSlot.Backpic.gotoAndPlay ("gear");
							}
							else if (slot == 7)
							{
								if (targetSlot.Backpic.currentLabel != "helmet")
									targetSlot.Backpic.gotoAndPlay ("helmet");
							}
							else if (slot != 0)
							{
								targetSlot.gotoAndPlay ("out");
							}
						}
					}				
				}

				SoundEvents.eventSoundPlay("menu_use");
				
				DraggedItem = null; 
				ItemIsDragging = false;
				DragMovie.visible = false;
				DragMovie.stopDrag(); 
			}*/
		}
		
		public	function eventBackpackToTradeCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				var q:uint = uint(text);
				if(q>0)
					TradeEvents.eventBackpackToTrade (MoveItem_SrcSlotID, MoveItem_slot, Math.min(q, MoveItem_Quantity));
			}
		}

		public	function eventTradeToBackpackCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				var q:uint = uint(text);
				if(q>0)
					TradeEvents.eventTradeToBackpack (MoveItem_SrcSlotID, MoveItem_slot, Math.min(q, MoveItem_Quantity));
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
			
			return; // drag & drop is disabled
			
			/*BackpackScrollerIsDragging = false;
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
			}*/
		}
		
		public function showUserTrade()
		{
			while(Trade.PlateUser.numChildren > 0)
				Trade.PlateUser.removeChildAt(0);
			
			var	survivor:Survivor = api.survivor;
			var slotOffsetX:int = 2;
			var	slotOffsetY:int = -14;
			var	slotWidth:int = 200;
			var	slotHeight:int = 99;

			for (var i = 0; i < UserTradeDB.length; i++)
			{
				var	backpackSlot = new warz.trade.InvBackpackSlot();

				backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
				backpackSlot.y = slotOffsetY + int ((i / 4)) * slotHeight;
				
				{
					var Name = "TradeSlot" + String(i + 1);
					
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startTradeDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					
					Trade.PlateUser.addChild(backpackSlot);
				}
				
				if (backpackSlot.currentFrame != 0)
					backpackSlot.gotoAndStop (0);
						
				if (backpackSlot.Type == "off")
				{
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startTradeDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
				}

				var	backItem:BackpackItem = UserTradeDB[i];				

				backpackSlot.BackpackItem = backItem;

				if (backItem.itemID>0)
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
							
							loadSlotIcon (item.Icon, backpackSlot.Pic, 3);
						}
						
						if (backpackSlot.PicFX)
						{
							if (backpackSlot.PicFX.numChildren > 0)
								backpackSlot.PicFX.removeChildAt(0);
								
							backpackSlot.PicFX.visible = false;
							
							loadSlotIcon (item.Icon, backpackSlot.PicFX, 3);
						}
					}

					{
						backpackSlot.Num.Text.text = backItem.quantity;
						backpackSlot.Num.visible = true;
					}
						
					{
						backpackSlot.Name.visible = true;					
						backpackSlot.Name.Text.text = item.Name;
					}
					
					backpackSlot.Item = item;
					backpackSlot.Focus.visible = false;

					backpackSlot.Type = "taken";
				}
				else
				{
					backpackSlot.Item = null;
					
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

					backpackSlot.Focus.visible = false;
					backpackSlot.Type = "available";
				}
			}
		}

		public function showOppositeTrade()
		{
			while(Trade.PlateOpponent.numChildren > 0)
				Trade.PlateOpponent.removeChildAt(0);
			
			var slotOffsetX:int = 2;
			var	slotOffsetY:int = -14;
			var	slotWidth:int = 200;
			var	slotHeight:int = 99;

			for (var i = 0; i < OppositeTradeDB.length; i++)
			{
				var	backpackSlot = new warz.trade.InvBackpackSlot();

				backpackSlot.x = slotOffsetX + (i % 4) * slotWidth;
				backpackSlot.y = slotOffsetY + int ((i / 4)) * slotHeight;
				
				{
					var Name = "OppositeSlot" + String(i + 1);
					
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					/*backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);*/
					backpackSlot.name = Name;
					backpackSlot.slotID = i;	
					
					Trade.PlateOpponent.addChild(backpackSlot);
				}
				
				if (backpackSlot.currentFrame != 0)
					backpackSlot.gotoAndStop (0);
						
				/*if (backpackSlot.Type == "off")
				{
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_DOWN, startBackpackDrag);
					backpackSlot.Btn.addEventListener(MouseEvent.MOUSE_UP, endBackpackDrag);
				}*/

				var	backItem:BackpackItem = OppositeTradeDB[i];				

				backpackSlot.BackpackItem = backItem;

				if (backItem.itemID>0)
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
							
							loadSlotIcon (item.Icon, backpackSlot.Pic, 3);
						}
						
						if (backpackSlot.PicFX)
						{
							if (backpackSlot.PicFX.numChildren > 0)
								backpackSlot.PicFX.removeChildAt(0);
								
							backpackSlot.PicFX.visible = false;
							
							loadSlotIcon (item.Icon, backpackSlot.PicFX, 3);
						}
					}

					{
						backpackSlot.Num.Text.text = backItem.quantity;
						backpackSlot.Num.visible = true;
					}
						
					{
						backpackSlot.Name.visible = true;					
						backpackSlot.Name.Text.text = item.Name;
					}
					
					backpackSlot.Item = item;
					backpackSlot.Focus.visible = false;

					backpackSlot.Type = "taken";
				}
				else
				{
					backpackSlot.Item = null;
					
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

					backpackSlot.Focus.visible = false;
					backpackSlot.Type = "available";
				}
			}
		}
		
		public function showBackpack ()
		{
			for(var i = 1; i <= 28; ++i)
			{
				var Name = "Slot"+i;
				Trade.Plate[Name].visible = false;
			}
			
			var	survivor:Survivor = api.survivor;
			var slotOffsetX:int = 2;
			var	slotOffsetY:int = 95;
			var	slotWidth:int = 200;
			var	slotHeight:int = 99;

// backpack info
			Trade.TypeShad.text = Trade.Type.text = survivor.getBackpackName();
			Trade.MaxWeightShad.text = Trade.MaxWeight.text = String (survivor.getBackpackMaxWeight ()) + " $FR_PAUSE_INVENTORY_LBS";
			Trade.WeightShad.text = Trade.Weight.text = survivor.weight.toFixed(1) + " $FR_PAUSE_INVENTORY_LBS";

			if (survivor.getBackpackSize () > 24)
			{
				BackpackScroller.visible = true;
				var me = this;
				
				BackpackScroller.Trigger.y = BackpackScroller.Field.y;
				BackpackScroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				
				BackpackScroller.ArrowDown.y = Trade.Plate.BackpackMask.height - 4;
				BackpackScroller.Field.height = (Trade.Plate.BackpackMask.height - (BackpackScroller.ArrowDown.height * 2));
			}
			else
			{
				BackpackScroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollBackpackItems);
				BackpackScroller.visible = false;
				backpackSlotMasks.y = 0;
			}

			for (i = 0; i < survivor.getBackpackSize(); i++)
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
						backpackSlot = new warz.trade.InvMainSlotLong();
						isNew = true;
					}

					backpackSlot.x = 2;
					backpackSlot.y = 95;
					backpackSlot.Key.Text.text = String (i + 1);

					if (backpackSlot.Btn.currentLabel != "long")
						backpackSlot.Btn.gotoAndPlay ("long");
				}
				else if (i == 1 || i == 6 || i == 7)
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.trade.InvMainSlotShort();
						isNew = true;
					}

					if (i == 1)
					{
						backpackSlot.x = 270;
					}
					else
					{
						backpackSlot.x = slotOffsetX + 466 + (i - 6) * 164;
					}

					backpackSlot.y = 96;
					
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
						backpackSlot = new warz.trade.InvBackpackHotSlot();
						isNew = true;
					}

					slotOffsetY = 115;
					backpackSlot.x = slotOffsetX + ((i - 2) % 4) * slotWidth;
					backpackSlot.y = slotOffsetY + slotHeight;
					
					if (backpackSlot.Key)
						backpackSlot.Key.Text.text = String (i + 1);
				}
				else 
				{
					if (!backpackSlot)
					{
						backpackSlot = new warz.trade.InvBackpackSlot();
						isNew = true;
					}

					slotOffsetY = 332;
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
					backpackSlot = new warz.trade.InvBackpackSlot();
					isNew = true;
				}

				slotOffsetY = 332;
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
			if(evt.currentTarget.parent.BackpackItem && !ItemIsDragging)
			{
				if(evt.currentTarget.parent.BackpackItem.Description != "")
				{
					var popUpStart:Point = new Point(0, 0);
					popUpStart = evt.currentTarget.parent.localToGlobal(popUpStart);
					Trade.SlotPopup.x = popUpStart.x + evt.currentTarget.parent.width;
					Trade.SlotPopup.y = popUpStart.y;
					Trade.SlotPopup.Text.text = evt.currentTarget.parent.BackpackItem.Description;
					Trade.SlotPopup.visible = true;
				}
			}
		}
		
		public	function backpackSlotRollOutFn(evt:Event)
		{
			if (ItemIsDragging || evt.currentTarget.parent.Item)
			{
				if (evt.currentTarget.parent != DraggedItem)
				{
					if (evt.currentTarget.parent.Type == "available" && evt.currentTarget.parent.name.indexOf("BackpackSlot")!=-1)
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
			if(evt.currentTarget.parent.BackpackItem && !ItemIsDragging)
			{
				Trade.SlotPopup.visible = false;
			}

		}
		
		public	function scrollBackpackItems(e:Event)
		{
			var	a = BackpackScroller.Field.y;
			var	b = BackpackScroller.Trigger.y;
			var	dist = (BackpackScroller.Field.y - BackpackScroller.Trigger.y);
			var	h = backpackSlotMasks.height;
			var	h1 = BackpackScroller.Field.height - BackpackScroller.Trigger.height;
			var	mh = Trade.Plate.BackpackMask.height;
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
