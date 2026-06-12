package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import caurina.transitions.Tweener;
	import warz.Utils;
	
	public class CreateSurvivorScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public 	var CreateSurv:MovieClip;
		public 	var api:warz.frontend.Frontend=null;
		
		//public	var	hardcore:int = 0;
		//public	var	hardcoreStr:Array = new Array ("$FR_CREATE_SURVIVOR_NORMAL", "$FR_CREATE_SURVIVOR_HARDCORE");

		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		public	var	heroSlots:MovieClip;
		public	var	created:Boolean;
		
		public	var	curHero:Hero;
		public	var	body:int;
		public	var	head:int;
		public	var	legs:int;
		
		public	var	actualGC:int = 0;
		public	var	actualGold:int = 0;
		public	var	actualCells:int = 0;

		public	var	visualGC:int = 0;
		public	var	visualGold:int = 0;
		public	var	visualCells:int = 0;

		public function CreateSurvivorScreen() {
			CreateSurv.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			CreateSurv.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			CreateSurv.BtnBack.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			CreateSurv.BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			CreateSurv.BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			CreateSurv.BtnCreate.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			//CreateSurv.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			//CreateSurv.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			//CreateSurv.BtnUnlock.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			CreateSurv.Character.ArrowRightZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowRightZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowRightZoneBtn1.addEventListener(MouseEvent.CLICK, arrowPressFn);

			CreateSurv.Character.ArrowRightZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowRightZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowRightZoneBtn2.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			CreateSurv.Character.ArrowRightZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowRightZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowRightZoneBtn3.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowLeftZoneBtn1.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowLeftZoneBtn2.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			CreateSurv.Character.ArrowLeftZoneBtn3.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			/*CreateSurv.Inputs.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event){if(evt.currentTarget.parent.currentFrameLabel != "inactive") evt.currentTarget.parent.gotoAndPlay("over");});
			CreateSurv.Inputs.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event){if(evt.currentTarget.parent.currentFrameLabel != "inactive") evt.currentTarget.parent.gotoAndPlay("out");});
			CreateSurv.Inputs.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, SlotPressFnNoState);
			
			CreateSurv.Inputs.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event){if(evt.currentTarget.parent.currentFrameLabel != "inactive") evt.currentTarget.parent.gotoAndPlay("over");});
			CreateSurv.Inputs.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event){if(evt.currentTarget.parent.currentFrameLabel != "inactive") evt.currentTarget.parent.gotoAndPlay("out");});
			CreateSurv.Inputs.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, SlotPressFnNoState);
			*/
			CreateSurv.Character.TitleHead.text = "$FR_CREATE_SURVIVOR_HEAD";
			CreateSurv.Character.TitleTorso.text = "$FR_CREATE_SURVIVOR_UPPER_BODY";
			CreateSurv.Character.TitleLegs.text = "$FR_CREATE_SURVIVOR_LEGS";
			
			//CreateSurv.Inputs.Mode.Text.text = hardcoreStr[hardcore];
			
			CreateSurv.Inputs.Name.InputText.addEventListener (FocusEvent.FOCUS_IN, inputFocusIn);
			CreateSurv.Inputs.Name.InputText.addEventListener (FocusEvent.FOCUS_OUT, inputFocusOut);
			
			CreateSurv.CreateSurvivorText.text = "$FR_CREATE_SURVIVOR_CREATE_NEW";
			
			var me = this;

			heroSlots = CreateSurv.CharSlots;

			Scroller = CreateSurv.Scroller;
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
			
			/*CreateSurv.CreateSurvModePopup.visible = false;
			CreateSurv.CreateSurvModePopup.Btn.addEventListener(MouseEvent.MOUSE_OVER, function() {});
			CreateSurv.CreateSurvModePopup.Btn.addEventListener(MouseEvent.MOUSE_OUT, function() {});
			CreateSurv.CreateSurvModePopup.Btn.addEventListener(MouseEvent.CLICK, function() {});
			CreateSurv.CreateSurvModePopup.BtnModeNormal.Text.Text.text = "$FR_NormalMode";
			CreateSurv.CreateSurvModePopup.BtnModeHard.Text.Text.text = "$FR_HardcoreMode";
			CreateSurv.CreateSurvModePopup.BtnContinue.Text.Text.text = "$FR_Continue";
			CreateSurv.CreateSurvModePopup.BtnModeNormal.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnCreatePopUpRollOverFn);
			CreateSurv.CreateSurvModePopup.BtnModeNormal.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnCreatePopUpRollOutFn);
			CreateSurv.CreateSurvModePopup.BtnModeNormal.Btn.addEventListener(MouseEvent.CLICK, BtnCreatePopUpPressFn);
			CreateSurv.CreateSurvModePopup.BtnModeHard.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnCreatePopUpRollOverFn);
			CreateSurv.CreateSurvModePopup.BtnModeHard.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnCreatePopUpRollOutFn);
			CreateSurv.CreateSurvModePopup.BtnModeHard.Btn.addEventListener(MouseEvent.CLICK, BtnCreatePopUpPressFn);
			CreateSurv.CreateSurvModePopup.BtnModeNormal.ActionFunction = function(){
													CreateSurv.CreateSurvModePopup.ModeSelected = 0;
													CreateSurv.CreateSurvModePopup.BtnModeHard.gotoAndPlay("out");
																				   }
			CreateSurv.CreateSurvModePopup.BtnModeHard.ActionFunction = function(){
													CreateSurv.CreateSurvModePopup.ModeSelected = 1;
													CreateSurv.CreateSurvModePopup.BtnModeNormal.gotoAndPlay("out");
																				   }
			CreateSurv.CreateSurvModePopup.BtnContinue.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnCreatePopUpRollOverFn);
			CreateSurv.CreateSurvModePopup.BtnContinue.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnCreatePopUpRollOutFn);
			CreateSurv.CreateSurvModePopup.BtnContinue.Btn.addEventListener(MouseEvent.CLICK, function()
												{
													CreateSurv.Inputs.Name.InputText.text = CreateSurv.CreateSurvModePopup.Name.text;
													CreateSurv.Inputs.Name.BackText.visible = CreateSurv.Inputs.Name.InputText.text.length == 0;
													if(CreateSurv.CreateSurvModePopup.ModeSelected==1)
													{
														hardcore = 1;
														CreateSurv.Inputs.ArrowRight.gotoAndStop("inactive");
														CreateSurv.Inputs.ArrowLeft.gotoAndStop(1);
														CreateSurv.Inputs.Mode.Text.text = hardcoreStr[hardcore];
													}													
													CreateSurv.CreateSurvModePopup.visible = false;
													
												});*/
			
		}
		/*public	function BtnCreatePopUpPressFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");
			evt.currentTarget.parent.ActionFunction();
		}
		public	function BtnCreatePopUpRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "pressed")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function BtnCreatePopUpRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "pressed")
				evt.currentTarget.parent.gotoAndPlay("out");
		}*/


		// event functions
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
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as CreateSurvivorScreen))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as CreateSurvivorScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public	function SlotPressFnNoState(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("pressed");
				
			var	p:MovieClip = evt.currentTarget.parent;
				
			while (!(p as CreateSurvivorScreen))
			{
				p = p.parent as MovieClip;
			}
			
			SoundEvents.eventSoundPlay("menu_click");
				
			(p as CreateSurvivorScreen).ActionFunction(evt.currentTarget.parent.name);
		}
		
		public	function arrowRollOverFn(evt:Event)
		{
			if(evt.currentTarget && evt.currentTarget.parent && evt.currentTarget.parent.currentFrameLabel!="inactive")
			{
				evt.currentTarget.parent.gotoAndPlay("over");
			}
		}
		
		public	function arrowRollOutFn(evt:Event)
		{
			if(evt.currentTarget && evt.currentTarget.parent && evt.currentTarget.parent.currentFrameLabel!="inactive")
			{
				evt.currentTarget.parent.gotoAndPlay("out");
			}
		}
		
		public	function arrowPressFn(evt:Event)
		{
			if(evt.currentTarget.Arrow.currentFrameLabel!="inactive")
			{
				var	p:MovieClip = (evt.currentTarget as MovieClip);
				
				while (!(p as CreateSurvivorScreen))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as CreateSurvivorScreen).ActionFunction(evt.currentTarget.name);
			}
		}

		public	function arrowRollOverFn2(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function arrowRollOutFn2(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public function Activate()
		{
			api = warz.frontend.Frontend.api;

			FrontEndEvents.eventRequestPlayerRender (2);

			this.gotoAndPlay("start");
			
			created = false;
			
			updateSurvivors ();
			
			updateHeros ();
			
			//hardcore = 0;
			//CreateSurv.Inputs.ArrowLeft.gotoAndStop("inactive");
			//CreateSurv.Inputs.ArrowRight.gotoAndStop(1);
			
			curHero = api.HeroDB[0];
			body = 0;
			head = 0;
			legs = 0;
			UpdateArrowStatus();

			if (curHero)
			{
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
			}
			
			var	heroSlot = heroSlots.getChildAt(0);
			if (api.getInventoryItemByID (curHero.itemID))
			{
				//CreateSurv.BtnUnlock.gotoAndStop("inactive");
				CreateSurv.BtnCreate.gotoAndStop(1);
				//CreateSurv.BtnUnlock.State = "active";
				CreateSurv.BtnCreate.State = "off";

				if (heroSlot.currentLabel != "pressed")
					heroSlot.gotoAndPlay("pressed");
			}
			else
			{
				//CreateSurv.BtnUnlock.gotoAndStop(1);
				CreateSurv.BtnCreate.gotoAndStop("inactive");
				//CreateSurv.BtnUnlock.State = "off";
				CreateSurv.BtnCreate.State = "active";

				if (heroSlot.currentLabel != "locked_pressed")
					heroSlot.gotoAndPlay("locked_pressed");
			}

			heroSlot.Active = true;
			
			CreateSurv.Inputs.Name.InputText.text = "";
			CreateSurv.Inputs.Name.InputText.maxChars = 16;
			CreateSurv.Inputs.Name.BackText.visible = true;

			/*CreateSurv.CreateSurvModePopup.visible = true;
			CreateSurv.CreateSurvModePopup.Name.text = "";
			CreateSurv.CreateSurvModePopup.ModeSelected = 0;
			CreateSurv.CreateSurvModePopup.BtnModeNormal.gotoAndPlay("pressed");*/

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function Deactivate()
		{
			//CreateSurv.CreateSurvModePopup.visible = false;
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);

			FrontEndEvents.eventRequestPlayerRender (0);
			
			if (!created)
				FrontEndEvents.eventCreateCancel ();
 
			this.gotoAndPlay("end");
		}
		
		public	function inputFocusIn (e:Event):void
		{
			CreateSurv.Inputs.Name.BackText.visible = false;
		}
		
		public	function inputFocusOut (e:Event):void
		{
			if (CreateSurv.Inputs.Name.InputText.text.length == 0)
				CreateSurv.Inputs.Name.BackText.visible = true;
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > CreateSurv.CharacterMask.x && 
				e.stageX < CreateSurv.CharacterMask.x + CreateSurv.CharacterMask.width + 45 &&
				e.stageY > CreateSurv.CharacterMask.y && 
				e.stageY < CreateSurv.CharacterMask.y + CreateSurv.CharacterMask.height)
			{
				var dist = (heroSlots.height - CreateSurv.CharacterMask.height) / 236;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}

		public function updateSurvivors ()
		{
			var selectedFirstEmptySlot = false;
			for(var i=0; i<5; ++i)
			{
				var slotName = "Slot"+(i+1);
				
				if (api.Survivors[i])
				{
					CreateSurv[slotName].State = "off";
					CreateSurv[slotName].gotoAndPlay("out");
					
					var	survivor:Survivor = api.Survivors[i];
					
					CreateSurv[slotName].Texts.Name.text = survivor.Name;
//					CreateSurv[slotName].Texts.Level.text = survivor.getLevel();
					CreateSurv[slotName].Texts.Status.text = survivor.getStatus();

					CreateSurv[slotName].Texts.StatusText.text = "$FR_Health";
					CreateSurv[slotName].Texts.TimeText.text = "$FR_SURVIVOR_SCREEN_CREATE_TIME_PLAYED";

					var	seconds:int = survivor.timeplayed % 60;
					var	mins:int = (survivor.timeplayed / 60) % 60;
					var	hours:int = (survivor.timeplayed / 3600) % 24;
					var	days:int = (survivor.timeplayed / 86400);
					var	time:String = "";
					
					time += String (days) + "D ";
					time += String (hours) + "H ";
					time += String (mins) + "M ";

					CreateSurv[slotName].Texts.Time.text = time;
					
					if (CreateSurv[slotName].Pic.Pic.numChildren > 0)
						CreateSurv[slotName].Pic.Pic.removeChildAt(0);
					
					var	hero:Hero = null;
					for(var j=0; j<api.HeroDB.length; ++j)
					{ 
						if(api.HeroDB[j].itemID == survivor.heroID)
						{
							hero = api.HeroDB[j];
							break;
						}
					}						
					loadSlotIcon (hero.icon2, CreateSurv[slotName].Pic.Pic, 2);

				}
				else
				{
					if(!selectedFirstEmptySlot)
						CreateSurv[slotName].gotoAndStop("empty_selected");
					else
						CreateSurv[slotName].gotoAndStop("empty");
					selectedFirstEmptySlot = true;
				}
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "BtnBack")
			{
				CreateSurv.BtnBack.State = "off";
				CreateSurv.BtnBack.gotoAndPlay("out");

				api.Main.showScreen("Survivors");
			}
			/*else if (button == "BtnUnlock")
			{
				if(CreateSurv.BtnUnlock.currentFrameLabel == "inactive")
					return;

				CreateSurv.BtnUnlock.State = "off";
				CreateSurv.BtnUnlock.gotoAndPlay("out");
				
				api.showCharacterUnlock (curHero.itemID);
			}*/
			else if (button == "BtnCreate")
			{
				if(CreateSurv.BtnCreate.currentFrameLabel == "inactive")
					return;
				if (api.getInventoryItemByID (curHero.itemID))
				{
					var	inventory:InventoryItem = api.getInventoryItemByID(curHero.itemID);
					
					if (inventory)
					{
						if (CreateSurv.Inputs.Name.InputText.text.length <= 3)
						{
							api.showInfoMsg("$FR_CREATE_CHARACTER_NAME_TOO_SHORT", true, "$FR_Create_Character");					
						}
						else
						{
							created = true;
							
							if (api.isDebug)
							{
								api.addClientSurvivor (CreateSurv.Inputs.Name.InputText.text, 100, 0, 0, /*hardcore*/0, 0, 0, 0, 0, 1, 100, 100, 100, 100, 16, 0, 0, 0, 0, "Good", "", true, 100);
								api.createCharSuccessful (api.Survivors.length - 1);
							}
							else
							{
								CreateSurv.Inputs.Name.InputText.text = warz.Utils.trimWhitespace(CreateSurv.Inputs.Name.InputText.text);
								FrontEndEvents.eventCreateCharacter(CreateSurv.Inputs.Name.InputText.text, curHero.itemID, /*hardcore*/0, head, body, legs);
							}
						}
					}
				}
				else
				{
					api.showInfoMsg("$FR_CREATE_CHARACTER_UNLOCK", true, "$FR_Create_Character");					
				}
				
				CreateSurv.BtnCreate.State = "off";
				CreateSurv.BtnCreate.gotoAndPlay("out");
			}
			/*else if (button == "ArrowLeft")
			{
				if (hardcore != 0)
					hardcore = 0;
					
				CreateSurv.Inputs.Mode.Text.text = hardcoreStr[hardcore];
				
				CreateSurv.Inputs.ArrowLeft.gotoAndStop("inactive");
				CreateSurv.Inputs.ArrowRight.gotoAndStop(1);
			}
			else if (button == "ArrowRight")
			{
				if (hardcore != 1)
					hardcore = 1;

				CreateSurv.Inputs.Mode.Text.text = hardcoreStr[hardcore];

				CreateSurv.Inputs.ArrowLeft.gotoAndStop(1);
				CreateSurv.Inputs.ArrowRight.gotoAndStop("inactive");
			}*/
			else if (button == "ArrowLeftZoneBtn1")
			{
				CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("pressed");
				head = Math.max(0, --head);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();			
			}
			else if (button == "ArrowLeftZoneBtn2")
			{
				CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("pressed");
				body = Math.max(0, --body);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowLeftZoneBtn3")
			{
				CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("pressed");
				legs = Math.max(0, --legs);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowRightZoneBtn1")
			{
				CreateSurv.Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("pressed");
				head = Math.min(++head, curHero.numHeads - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();				
			}
			else if (button == "ArrowRightZoneBtn2")
			{
				CreateSurv.Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("pressed");
				body = Math.min(++body, curHero.numBodys - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowRightZoneBtn3")
			{
				CreateSurv.Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("pressed");
				legs = Math.min(++legs, curHero.numLegs - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
		}
		
		public function UpdateArrowStatus()
		{
			if(head == 0)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numHeads > 1 && CreateSurv.Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_out");
			}
			if(head == curHero.numHeads-1)
			{
				if(curHero.numHeads > 1 && CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_in");
			}
			if(head > 0 && head < curHero.numHeads-1)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_out");
			}
			if(body == 0)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numBodys>1 && CreateSurv.Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_out");
			}
			if(body == curHero.numBodys-1)
			{
				if(curHero.numBodys>1 && CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_in");
			}	
			if(body > 0 && body < curHero.numBodys-1)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_out");
			}
			if(legs == 0)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numLegs>1 && CreateSurv.Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_out");
			}
			if(legs == curHero.numLegs-1)
			{
				if(curHero.numLegs>1 && CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel != "inactive")
					CreateSurv.Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_in");
			}	
			if(legs > 0 && legs < curHero.numLegs-1)
			{
				if(CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_out");
				if(CreateSurv.Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel == "inactive")
					CreateSurv.Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_out");
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
				var endY = Scroller.Field.height - Scroller.Trigger.height+26;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}
		
		public function updateHeros ()
		{
			if (api.HeroDB.length > 9)
			{
				Scroller.visible = true;
				var me = this;
				
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = CreateSurv.CharacterMask.height - 4;
				Scroller.Field.height = (CreateSurv.CharacterMask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				heroSlots.y = 222.95;
			}

			while (heroSlots.numChildren > 0)
			{
				heroSlots.removeChildAt(0);
			}			

			for (var i = 0; i < api.HeroDB.length; i++)
			{
				var	hero:Hero = api.HeroDB[i];
				var heroSlot = new warz.frontend.CreateSurvCharSlot();
				
				heroSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, heroRollOverFn);
				heroSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, heroRollOutFn);
				heroSlot.Btn.addEventListener(MouseEvent.CLICK, heroRollClickFn);

				heroSlot.x = (i % 3) * 172.45;
				heroSlot.y = int (i / 3) * 236;
				heroSlot.hero = hero;
				
				if (heroSlot.Pic.numChildren > 0)
					heroSlot.Pic.removeChildAt(0);

				heroSlots.addChild(heroSlot);
				
				loadSlotIcon (hero.icon, heroSlot.Pic, 1);
				
				heroSlot.Texts.StatText1.text = hero.name;
				
				var	storeItem:StoreItem = api.getStoreItemByID (hero.itemID);
				var	inventory:InventoryItem = api.getInventoryItemByID(hero.itemID);
				
				if (inventory)
				{
					if (heroSlot.currentLabel != "out")
						heroSlot.gotoAndPlay("out");
				}
				else
				{
					heroSlot.BtnUnlock.addEventListener(MouseEvent.MOUSE_OVER, unlockRollOverFn);
					heroSlot.BtnUnlock.addEventListener(MouseEvent.MOUSE_OUT, unlockRollOutFn);
					heroSlot.BtnUnlock.addEventListener(MouseEvent.CLICK, unlockRollClickFn);
					if (heroSlot.currentLabel != "locked_out")
						heroSlot.gotoAndPlay("locked_out");
				}				
				
				heroSlot.StoreItem = storeItem;
				heroSlot.Inventory = inventory;
			}
		}
		
		public	function heroRollOverFn(evt:Event)
		{
			if (evt.currentTarget.parent.Active == true)
				return;
				
			if (evt.currentTarget.parent.Inventory)
			{
				if (evt.currentTarget.parent.currentLabel != "over")
					evt.currentTarget.parent.gotoAndPlay("over");
			}
			else
			{
				if (evt.currentTarget.parent.currentLabel != "locked_over")
					evt.currentTarget.parent.gotoAndPlay("locked_over");
			}				
		}
		
		public	function heroRollOutFn(evt:Event)
		{
			if (evt.currentTarget.parent.Active == true)
				return;
				
			if (evt.currentTarget.parent.Inventory)
			{
				if (evt.currentTarget.parent.currentLabel != "out")
					evt.currentTarget.parent.gotoAndPlay("out");
			}
			else
			{
				if (evt.currentTarget.parent.currentLabel != "locked_out")
					evt.currentTarget.parent.gotoAndPlay("locked_out");
			}				
		}

		public	function heroRollClickFn(evt:Event)
		{
			if(curHero == evt.currentTarget.parent.hero)
				return;
			
			for (var a:int = 0; a < heroSlots.numChildren; a++)
			{
				var	heroslot = heroSlots.getChildAt(a);
				
				heroslot.BtnUnlock.visible = false;
				heroslot.BtnInfo.visible = false;
				if (heroslot.Inventory)
				{
					if (heroslot.currentLabel != "out")
						heroslot.gotoAndPlay("out");
				}
				else
				{
					if (heroslot.currentLabel != "locked_out")
						heroslot.gotoAndPlay("locked_out");
				}			
				
				heroslot.Active = false;
			}			

			curHero = evt.currentTarget.parent.hero;
			body = 0;
			head = 0;
			legs = 0;
			UpdateArrowStatus();

			if (api.getInventoryItemByID (curHero.itemID))
			{
				//CreateSurv.BtnUnlock.gotoAndStop("inactive");
				CreateSurv.BtnCreate.gotoAndStop(1);
				//CreateSurv.BtnUnlock.State = "active";
				CreateSurv.BtnCreate.State = "off";

			}
			else
			{
				//CreateSurv.BtnUnlock.gotoAndStop(1);
				CreateSurv.BtnCreate.gotoAndStop("inactive");
				//CreateSurv.BtnUnlock.State = "off";
				CreateSurv.BtnCreate.State = "active";
			}
			
			FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);

			if (evt.currentTarget.parent.Inventory)
			{
				if (evt.currentTarget.parent.currentLabel != "pressed")
					evt.currentTarget.parent.gotoAndPlay("pressed");
			}
			else
			{
				if (evt.currentTarget.parent.currentLabel != "locked_pressed")
					evt.currentTarget.parent.gotoAndPlay("locked_pressed");
			}			
			
			evt.currentTarget.parent.BtnUnlock.visible = true;
			evt.currentTarget.parent.BtnInfo.visible = true;
			
			evt.currentTarget.parent.Active = true;
		}
		public	function unlockRollOverFn(evt:Event)
		{
			if (evt.currentTarget.parent.Active == true)
				return;
				
			if (evt.currentTarget.currentLabel != "over")
				evt.currentTarget.gotoAndPlay("over");
		}
		
		public	function unlockRollOutFn(evt:Event)
		{
			if (evt.currentTarget.parent.Active == true)
				return;
				
			if (evt.currentTarget.currentLabel != "out")
				evt.currentTarget.gotoAndPlay("out");
		}

		public	function unlockRollClickFn(evt:Event)
		{
			if(curHero != evt.currentTarget.parent.hero)
				return;
			if (evt.currentTarget.parent.Active != true)
				return;

			if (evt.currentTarget.currentLabel != "pressed")
				evt.currentTarget.gotoAndPlay("pressed");
			
			SoundEvents.eventSoundPlay("menu_click");

			api.showCharacterUnlock (curHero.itemID);
			
		}		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = heroSlots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = CreateSurv.CharacterMask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 236);
			
			if (heroSlots.y != (222.95+236*step))
			{
				Tweener.addTween(heroSlots, {y:222.95+(236 * step), time:api.tweenDelay, transition:"linear"});
			}

//			heroSlots.y = dist * h;
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
			
			slotWidth = 256;
			slotHeight = 256;
			slotX = 0;
			slotY = 0;
			
			if(dat.slotType == 2)
				slotHeight = 128;

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}
		
		public function buyItemSuccessful ()
		{
			updateHeros ();
			
			if (api.getInventoryItemByID (curHero.itemID))
			{
				//CreateSurv.BtnUnlock.gotoAndStop("inactive");
				CreateSurv.BtnCreate.gotoAndStop(1);
				//CreateSurv.BtnUnlock.State = "active";
				CreateSurv.BtnCreate.State = "off";
			}
			else
			{
				//CreateSurv.BtnUnlock.gotoAndStop(1);
				CreateSurv.BtnCreate.gotoAndStop("inactive");
				//CreateSurv.BtnUnlock.State = "off";
				CreateSurv.BtnCreate.State = "active";

			}
		}
	}
}
