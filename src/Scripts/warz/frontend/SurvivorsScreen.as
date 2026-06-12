package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.utils.Timer;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import scaleform.clik.controls.NumericStepper;
	
	public class SurvivorsScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var Survivors:MovieClip;
		public var api:warz.frontend.Frontend=null;
		
		private var deadTimer:Timer;
		
		public function SurvivorsScreen() {
			for(var i=1; i<6; ++i)
			{
				var Name = "Slot"+i;
				Survivors[Name].Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				Survivors[Name].Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				Survivors[Name].Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			}
			
			Survivors.PremiumAcc.visible = false;
			Survivors.BtnUpgtrial.visible = false;
			Survivors.BtnUpgtrial.Text.text = "$FR_UpgradeTrialAccount";
			Survivors.BtnUpgtrial.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.BtnUpgtrial.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.BtnUpgtrial.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			Survivors.PopupLastMap.visible = false;
			
			Survivors.BtnLastMap.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn2);
			Survivors.BtnLastMap.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn2);
			Survivors.BtnLastMap.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn2);

			Survivors.PopupLastMap.MapBtnClose.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn2);
			Survivors.PopupLastMap.MapBtnClose.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn2);
			Survivors.PopupLastMap.MapBtnClose.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn2);

			Survivors.Inventory.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.Inventory.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.Inventory.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			Survivors.Skill_Tree.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.Skill_Tree.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.Skill_Tree.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			Survivors.ChangeOutfit.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.ChangeOutfit.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.ChangeOutfit.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			Survivors.ChangeName.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.ChangeName.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.ChangeName.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			Survivors.BtnDeleteSlot.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			Survivors.BtnDeleteSlot.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			Survivors.BtnDeleteSlot.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			Survivors.SelectSurvivor.text = "$FR_SURVIVOR_SCREEN_Select_Survivor";
			//Survivors.BtnCreateSlot.Text.Text.text = "$FR_SURVIVOR_SCREEN_CREATE_NEW_SURVIVOR";
			Survivors.BtnDeleteSlot.Text.Text.text = "$FR_DELETE";
			
			Survivors.LevelTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_EXPERIENCE";
			Survivors.SurvivedTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_SURVIVED";
			Survivors.KilledZombTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_KILLED_ZOMBIES";
			Survivors.KilledBandTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_KILLED_BANDITS";
			Survivors.KilledCivTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_KILLED_CIVILIANS";
			Survivors.AlignmentTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_ALIGNMENT";
			Survivors.LastMapTitle.text = "$FR_SURVIVOR_SCREEN_CREATE_LAST_MAP";
		}
		
		// event functions
		private function SlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function SlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function SlotPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if (evt.currentTarget.parent.name.indexOf("Slot") != -1)
				{
					for(var i=1; i<6; ++i)
					{
						var Name = "Slot"+i;
						if(Survivors[Name].State == "active")
						{
							Survivors[Name].State = "off";
							Survivors[Name].gotoAndPlay("out");
						}
					}
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as SurvivorsScreen))
				{
					p = p.parent as MovieClip;
				}
				
				(p as SurvivorsScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		private function SlotRollOverFn2(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("empty_over");
		}
		private function SlotRollOutFn2(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("empty_out");
		}
		private function SlotPressFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if (evt.currentTarget.parent.name.indexOf("Slot") != -1)
				{
					for(var i=1; i<6; ++i)
					{
						var Name = "Slot"+i;
						if(Survivors[Name].State == "active")
						{
							Survivors[Name].State = "off";
							Survivors[Name].gotoAndPlay("out");
						}
					}
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as SurvivorsScreen))
				{
					p = p.parent as MovieClip;
				}
				
				(p as SurvivorsScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			if (api.SelectedChar == -1)
			{
				api.SelectedChar = 0;
			}

			this.gotoAndPlay("start");
			
			FrontEndEvents.eventRequestPlayerRender (1);
			
			updateSurvivors ();

			api.setButtonText(Survivors.Inventory.Text, "$FR_Inventory");
			api.setButtonText(Survivors.Skill_Tree.Text, "$FR_SkillTree");
			api.setButtonText(Survivors.ChangeOutfit.Text, "$FR_ChangeOutfit");
			api.setButtonText(Survivors.ChangeName.Text, "$FR_ChangeName");
			
			
			//api.showInfoMsg("ions of the Terms of Use at <font color=\"#0000FF\"><a href=\"event:clickLink10\">http://www.infestationmmo.com/tos.html</a></font>. Customer Support is unable to assist", true);
		}
		
		public function Deactivate()
		{
			FrontEndEvents.eventRequestPlayerRender (0);
			
			if (Survivors.CharStatus.ReviveBtn)
			{
				Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
				Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
				Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
			}

			this.gotoAndPlay("end");
		}
		
		public function addSurvivorPinToMap(pos_x:Number, pos_y:Number, Name:String)
		{
			var pin = new warz.frontend.SurvivorsPopUpLastMapIconPin();
			pin.x = Survivors.PopupLastMap.PinMap.x + pos_x*Survivors.PopupLastMap.PinMap.width;
			pin.y = Survivors.PopupLastMap.PinMap.y + pos_y*Survivors.PopupLastMap.PinMap.height;
			pin.Username.text = Name;
			
			Survivors.PopupLastMap.PinList.addChild(pin);
		}
		
		public function loadSurvivorMap(iconPath:String)
		{
			var	imageLoader:ImageLoader = new ImageLoader (iconPath, loadMapCallback, null)
		}

		public	function loadMapCallback (bitmap:Bitmap, dat:*)
		{
			while (Survivors.PopupLastMap.Minimap.numChildren > 0)
				Survivors.PopupLastMap.Minimap.removeChildAt(0);
				
			bitmap = Layout.stretch (bitmap, 967, 967, bitmap.width, bitmap.height, "exact");
			bitmap.x = 0;
			bitmap.y = 0;
			
			Survivors.PopupLastMap.Minimap.addChild (bitmap);
		}

		public function showSurvivorsMap()
		{
			if (Survivors.PopupLastMap.PinList.numChildren > 0)
				Survivors.PopupLastMap.PinList.removeChildAt(0);
						
			FrontEndEvents.eventShowSurvivorsMap();
			//Survivors.PopupLastMap.visible = true;
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "BtnLastMap")
			{
				Survivors.BtnLastMap.State = "off";
				Survivors.BtnLastMap.gotoAndPlay("empty_out");
				
				showSurvivorsMap();
			}
			else if( button=="MapBtnClose")
			{
				Survivors.PopupLastMap.MapBtnClose.gotoAndPlay("empty_out");
				Survivors.PopupLastMap.visible = false;
			}
			else if (button == "BtnUpgtrial")
			{
				Survivors.BtnUpgtrial.State = "off";
				Survivors.BtnUpgtrial.gotoAndPlay("out");
				
				//api.showTrialUpgradeWindow();
				FrontEndEvents.eventTrialRequestUpgrade();
			}
			else if (button == "BtnDeleteSlot")
			{
				Survivors.BtnDeleteSlot.State = "off";
				Survivors.BtnDeleteSlot.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg ("$FR_DeleteCharacterConf", "$FR_DeleteCharacter", deleteCallback);
			}
			else if (button.indexOf("Slot") != -1)
			{
// display survivor				
				var	slot = int (button.slice (4)) - 1;
				
				if (slot < api.Survivors.length)
					DisplaySurvivor (slot);
				else
					api.showCreateSurvivor ();
					
			}
			else if (button == "Inventory")
			{
				Survivors.Inventory.State = "off";
				Survivors.Inventory.gotoAndPlay("out");
				api.showInventory ();
			}
			else if (button == "Skill_Tree")
			{
				Survivors.Skill_Tree.State = "off";
				Survivors.Skill_Tree.gotoAndPlay("out");
				api.showSkillTree ();
			}
			else if (button == "ChangeOutfit")
			{
				Survivors.ChangeOutfit.State = "off";
				Survivors.ChangeOutfit.gotoAndPlay("out");
				api.Main.showScreen("ChangeOutfit");
			}
			else if (button == "ChangeName")
			{
				Survivors.ChangeName.State = "off";
				Survivors.ChangeName.gotoAndPlay("out");
				api.Main.MsgBox.showInfoInputMsg("$FR_ChangeNameDesc "+api.ChangeName_Price, "", changeNameCallback, 16);
			}
			else if (button == "ReviveBtn")
			{
				Survivors.CharStatus.ReviveBtn.State = "off";
				Survivors.CharStatus.ReviveBtn.gotoAndPlay("out");
				
				if (api.isDebug)
				{
					api.reviveCharSuccessful ();
				}
				else
				{
					FrontEndEvents.eventReviveChar();
				}
			}
		}
		
		public	function changeNameCallback (state:Boolean, text:String=""):void
		{
			//trace("s="+state+", text="+text);
			if (state)
			{
				if(text.length > 0)
					FrontEndEvents.eventRenameCharacter(text);
			}
		}
		
		public	function deleteCallback (state:Boolean):void
		{
			if (state)
			{
				if (api.isDebug)
					api.deleteCharSuccessful ();
				else 
					FrontEndEvents.eventDeleteChar();
			}
		}
		
		public function updateSurvivors ()
		{
			if (api.Survivors.length > 0)
			{
				if (Survivors.BtnDeleteSlot.currentFrame != 0)
					Survivors.BtnDeleteSlot.gotoAndStop (0);

				Survivors.BtnDeleteSlot.State = "off";
			}
			else
			{
				if (Survivors.BtnDeleteSlot.currentLabel != "inactive")
					Survivors.BtnDeleteSlot.gotoAndPlay ("inactive");
				
				Survivors.BtnDeleteSlot.State = "active";
			}
			
			for(var i=0; i<5; ++i)
			{
				var slotName = "Slot"+(i+1);
				Survivors[slotName].State = "off";
				
				var	survivor:Survivor = api.Survivors[i];
				
				Survivors[slotName].Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn2);
				Survivors[slotName].Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn2);
				Survivors[slotName].Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn2);
				
				if (survivor)
				{
					Survivors[slotName].gotoAndPlay("out");
					
					Survivors[slotName].Texts.Name.text = survivor.Name;
//					Survivors[slotName].Texts.Level.text = survivor.getLevel ();
					Survivors[slotName].Texts.Status.text = survivor.getStatus();
					
					var	seconds:int = survivor.timeplayed % 60;
					var	mins:int = (survivor.timeplayed / 60) % 60;
					var	hours:int = (survivor.timeplayed / 3600) % 24;
					var	days:int = (survivor.timeplayed / 86400);
					var	time:String = "";
					
					if(api.m_Language == "russian")
					{
						time += String (days) + "Д ";
						time += String (hours) + "Ч ";
						time += String (mins) + "М ";
					}
					else
					{
						time += String (days) + "D ";
						time += String (hours) + "H ";
						time += String (mins) + "M ";
					}
						
					Survivors[slotName].Texts.Time.text = time;
					
					Survivors[slotName].Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
					Survivors[slotName].Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
					Survivors[slotName].Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
//					Survivors[slotName].Texts.LevelText.text = "$FR_SURVIVOR_SCREEN_CREATE_LevelText";
					Survivors[slotName].Texts.StatusText.text = "$FR_Health";
					Survivors[slotName].Texts.TimeText.text = "$FR_SURVIVOR_SCREEN_CREATE_TIME_PLAYED";
					
					if (Survivors[slotName].Pic.Pic.numChildren > 0)
						Survivors[slotName].Pic.Pic.removeChildAt(0);
					
					var	hero:Hero = null;
					for(var j=0; j<api.HeroDB.length; ++j)
					{ 
						if(api.HeroDB[j].itemID == survivor.heroID)
						{
							hero = api.HeroDB[j];
							break;
						}
					}					
					loadSlotIcon (hero.icon2, Survivors[slotName].Pic.Pic, 2);
				}
				else
				{
					Survivors[slotName].gotoAndStop("empty");
					
					Survivors[slotName].Texts2.Name.text = "$FR_SURVIVOR_SCREEN_CREATE_EMPTY";
					Survivors[slotName].Texts2.LevelText.text = "$FR_SURVIVOR_SCREEN_CREATE_CLICK_TO_CREATE";
					
					Survivors[slotName].Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn2);
					Survivors[slotName].Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn2);
					Survivors[slotName].Btn.addEventListener(MouseEvent.CLICK, SlotPressFn2);
				}
			}
			
			DisplaySurvivor (api.SelectedChar);
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
			slotHeight = 128;
			slotX = 0;
			slotY = 0;

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}

		public	function updateDeadTimer (seconds:int, percent:int, reviveBtnVis:Boolean)
		{
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			if (survivor.alive == 0)
			{
				if (Survivors.CharStatus.RevCircle)
				{
					Survivors.CharStatus.RevCircle.gotoAndStop (percent);
				}
				
				if (Survivors.CharStatus.Time)
				{
					var	mins:int = (seconds / 60) % 60;
					var	hours:int = (seconds / 3600);
					
					var text:String = "";
					if(hours < 10)
						text += "0";
					text += String (hours) + ":";
					if (mins < 10)
						text += "0";
					text += String (int (mins % 60));
					
					Survivors.CharStatus.Time.text = text;
				}
				
				Survivors.CharStatus.ReviveBtn.visible = reviveBtnVis;
			}
		}

		public function DisplaySurvivor (slot:int)
		{
			if (api.Survivors.length == 0)
			{
				Survivors.CharStatus.visible = false;
				Survivors.Level.text = "";
				Survivors.Survived.text = "";
				Survivors.KilledZomb.text = "";
				Survivors.KilledBand.text = "";
				Survivors.KilledCiv.text = "";
				Survivors.Alignment.text = "";
				Survivors.LastMap.text = "";
				Survivors.CharName.text = "";
				
				Survivors.Inventory.visible = false;
				Survivors.Skill_Tree.visible = false;
				Survivors.ChangeOutfit.visible = false;
				
				Survivors.LevelTitle.visible = false;
				Survivors.SurvivedTitle.visible = false;
				Survivors.KilledZombTitle.visible = false;
				Survivors.KilledBandTitle.visible = false;
				Survivors.KilledCivTitle.visible = false;
				Survivors.AlignmentTitle.visible = false;
				Survivors.LastMapTitle.visible = false;
			}
			
			if (slot < 0 || slot >= api.Survivors.length)
			{
				return;
			}
				
			api.SelectedChar = slot;					
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			
			if (!survivor)
			{
				return;
			}

			Survivors.Inventory.visible = true;
			Survivors.Skill_Tree.visible = true;
			Survivors.ChangeOutfit.visible = true;
			Survivors.LevelTitle.visible = true;
			Survivors.SurvivedTitle.visible = true;
			Survivors.KilledZombTitle.visible = true;
			Survivors.KilledBandTitle.visible = true;
			Survivors.KilledCivTitle.visible = true;
			Survivors.AlignmentTitle.visible = true;
			Survivors.LastMapTitle.visible = true;

			Survivors["Slot" + String (api.SelectedChar + 1)].State = "active";
			Survivors["Slot" + String (api.SelectedChar + 1)].gotoAndPlay("pressed");
			
			Survivors.CharStatus.visible = true;

			if(survivor.health==100)
			{		
				if (Survivors.CharStatus.ReviveBtn)
				{
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
				}

				if (Survivors.CharStatus.currentLabel != "ready")
				{
					Survivors.CharStatus.gotoAndPlay ("ready");
				}
			}
			else if(survivor.health==0 && survivor.alive == 0)
			{
				if (survivor.hardcore == 1) // permanently dead
				{
					if (Survivors.CharStatus.currentLabel != "perm_dead")
					{
						Survivors.CharStatus.gotoAndPlay ("perm_dead");
					}
				}
				else
				{
					if (Survivors.CharStatus.currentLabel != "dead")
					{
						Survivors.CharStatus.gotoAndPlay ("dead");
					}
							
					if (Survivors.CharStatus.ReviveBtn)
					{
						Survivors.CharStatus.ReviveBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
						Survivors.CharStatus.ReviveBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
						Survivors.CharStatus.ReviveBtn.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
							
						Survivors.CharStatus.ReviveBtn.visible = true;
					}
	
					if (Survivors.CharStatus.RevCircle)
					{
						Survivors.CharStatus.RevCircle.gotoAndStop (0);
					}
						
					Survivors.CharStatus.DeadTitle.text = "$FR_SURVIVOR_SCREEN_DEAD";
					Survivors.CharStatus.ReviveIn.text = "$FR_SURVIVOR_SCREEN_REVIVE_IN";
					Survivors.CharStatus.Hours.text = "$FR_SURVIVOR_REVIVE_SCREEN_HOURS";
					Survivors.CharStatus.Mins.text = "$FR_SURVIVOR_REVIVE_SCREEN_MINS";
				}
			}
			else
			{
				if (Survivors.CharStatus.currentLabel != "wounded")
				{
					Survivors.CharStatus.gotoAndPlay ("wounded");
				}
					
				if (Survivors.CharStatus.ReviveBtn)
				{
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
					Survivors.CharStatus.ReviveBtn.Btn.removeEventListener(MouseEvent.CLICK, SlotPressFn);
						
					Survivors.CharStatus.ReviveBtn.visible = false;
				}
			}

			Survivors.CharName.text = survivor.Name;

			var	seconds:int = survivor.timeplayed % 60;
			var	mins:int = (survivor.timeplayed / 60) % 60;
			var	hours:int = (survivor.timeplayed / 3600) % 24;
			var	days:int = (survivor.timeplayed / 86400);
			var	time:String = "";
			
			if(api.m_Language == "russian")
			{
				time += String (days) + "Д ";
				time += String (hours) + "Ч ";
				time += String (mins) + "М ";
			}
			else
			{
				time += String (days) + "D ";
				time += String (hours) + "H ";
				time += String (mins) + "M ";
			}

			Survivors.Level.text = survivor.getLevel();
			Survivors.Survived.text = time;
			Survivors.KilledZomb.text = String (survivor.zombiesKilled);
			Survivors.KilledBand.text = String (survivor.banditsKilled);
			Survivors.KilledCiv.text = String (survivor.civiliansKilled);
			Survivors.Alignment.text = survivor.alignment;
			Survivors.LastMap.text = survivor.lastMap;
		}
	}
}

