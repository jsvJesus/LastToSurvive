package warz.frontend {
	import flash.display.MovieClip;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import caurina.transitions.Tweener;
	import warz.Utils;

	public class CreateClanScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var CreateClan:MovieClip;
		
		private var newClan_nameColor = 8;
		private var newClan_tagColor = 8;
		private var newClan_currentIconID = 0;
		private var newClan_selectingIconID = 0;
		
		public	function CreateClanScreen ()
		{
			visible = false;
			var me = this;
			
			CreateClan.Tab1.Text.Text.text = "$FR_CLANS";
			CreateClan.Tab2.Text.Text.text = "$FR_CREATECLAN";
			CreateClan.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
			CreateClan.Tab3.visible = false;
			
			CreateClan.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			CreateClan.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			CreateClan.Tab1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			CreateClan.Tab1.ActionFunction = function() {api.Main.showScreen("Clans");}

			CreateClan.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			CreateClan.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			CreateClan.Tab2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			CreateClan.Tab2.ActionFunction = function() {}

			CreateClan.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			CreateClan.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			CreateClan.Tab3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			CreateClan.Tab3.ActionFunction = function() {}

			CreateClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			CreateClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			CreateClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			CreateClan.MenuBtnLeader.ActionFunction = function() {api.Main.showScreen("Leaderboard");}
			CreateClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			CreateClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			CreateClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			CreateClan.MenuBtnFriends.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			CreateClan.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			CreateClan.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			CreateClan.MenuBtnNews.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			CreateClan.MenuBtnNews.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}

			CreateClan.MenuBtnFriends.visible = false;
			CreateClan.MenuBtnNews.visible = false;
			
			CreateClan.BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			CreateClan.BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			CreateClan.BtnCreate.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			CreateClan.BtnCreate.ActionFunction = function() 
			{
				CreateClan.ClanName.text = warz.Utils.trimWhitespace(CreateClan.ClanName.text);
				CreateClan.ClanTag.text = warz.Utils.trimWhitespace(CreateClan.ClanTag.text);
				if(CreateClan.ClanName.length < 4)
				{
					api.showInfoMsg("$FR_ClanNameNotLongEnough", true);
					return;
				}
				if(CreateClan.ClanTag.length != 4)
				{
					api.showInfoMsg("$FR_ClanTagLengthIncorrect", true);
					return;
				}
				FrontEndEvents.eventCreateClan(CreateClan.ClanName.text, CreateClan.ClanTag.text, CreateClan.ClanDesc.text, me.newClan_nameColor,
											   me.newClan_tagColor, me.newClan_currentIconID);
			}
			
			CreateClan.IconBlock.Btn.Text.Text.text = "$FR_ClanChangeIcon";
			CreateClan.IconBlock.Btn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			CreateClan.IconBlock.Btn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			CreateClan.IconBlock.Btn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			CreateClan.IconBlock.Btn.ActionFunction = function() 
			{
				if(CreateClan.PicBrowser.currentFrame == 10) // active
				{
					CreateClan.PicBrowser.gotoAndPlay("out");
					CreateClan.IconBlock.Btn.Text.Text.text = "$FR_ClanChangeIcon";
					newClan_currentIconID = newClan_selectingIconID;
					if (CreateClan.IconBlock.Pic.numChildren > 0)
						CreateClan.IconBlock.Pic.removeChildAt(0);
					loadSlotIcon(api.myClanIcons[newClan_currentIconID], CreateClan.IconBlock.Pic, 0);
				}
				else
				{
					CreateClan.PicBrowser.gotoAndPlay("start");
					CreateClan.IconBlock.Btn.Text.Text.text = "$FR_Apply";
				}
			}
			
			CreateClan.ColorBlocksName.ColorBlock1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock4.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock5.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock6.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock6.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock6.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock7.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock7.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock7.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			CreateClan.ColorBlocksName.ColorBlock8.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksName.ColorBlock8.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksName.ColorBlock8.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);

			CreateClan.ColorBlocksTag.ColorBlock1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock4.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock5.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock6.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock6.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock6.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock7.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock7.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock7.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);
			CreateClan.ColorBlocksTag.ColorBlock8.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			CreateClan.ColorBlocksTag.ColorBlock8.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			CreateClan.ColorBlocksTag.ColorBlock8.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn4);

			CreateClan.ColorBlocksName.ColorBlock1.Color.gotoAndStop(1);
			CreateClan.ColorBlocksName.ColorBlock2.Color.gotoAndStop(2);			
			CreateClan.ColorBlocksName.ColorBlock3.Color.gotoAndStop(3);			
			CreateClan.ColorBlocksName.ColorBlock4.Color.gotoAndStop(4);			
			CreateClan.ColorBlocksName.ColorBlock5.Color.gotoAndStop(5);			
			CreateClan.ColorBlocksName.ColorBlock6.Color.gotoAndStop(6);			
			CreateClan.ColorBlocksName.ColorBlock7.Color.gotoAndStop(7);			
			CreateClan.ColorBlocksName.ColorBlock8.Color.gotoAndStop(8);			

			CreateClan.ColorBlocksTag.ColorBlock1.Color.gotoAndStop(1);
			CreateClan.ColorBlocksTag.ColorBlock2.Color.gotoAndStop(2);			
			CreateClan.ColorBlocksTag.ColorBlock3.Color.gotoAndStop(3);			
			CreateClan.ColorBlocksTag.ColorBlock4.Color.gotoAndStop(4);			
			CreateClan.ColorBlocksTag.ColorBlock5.Color.gotoAndStop(5);			
			CreateClan.ColorBlocksTag.ColorBlock6.Color.gotoAndStop(6);			
			CreateClan.ColorBlocksTag.ColorBlock7.Color.gotoAndStop(7);			
			CreateClan.ColorBlocksTag.ColorBlock8.Color.gotoAndStop(8);			
			
			CreateClan.ColorBlocksTag.ColorBlock1.colorID = CreateClan.ColorBlocksName.ColorBlock1.colorID = 1;
			CreateClan.ColorBlocksTag.ColorBlock2.colorID = CreateClan.ColorBlocksName.ColorBlock2.colorID = 2;
			CreateClan.ColorBlocksTag.ColorBlock3.colorID = CreateClan.ColorBlocksName.ColorBlock3.colorID = 3;
			CreateClan.ColorBlocksTag.ColorBlock4.colorID = CreateClan.ColorBlocksName.ColorBlock4.colorID = 4;
			CreateClan.ColorBlocksTag.ColorBlock5.colorID = CreateClan.ColorBlocksName.ColorBlock5.colorID = 5;
			CreateClan.ColorBlocksTag.ColorBlock6.colorID = CreateClan.ColorBlocksName.ColorBlock6.colorID = 6;
			CreateClan.ColorBlocksTag.ColorBlock7.colorID = CreateClan.ColorBlocksName.ColorBlock7.colorID = 7;
			CreateClan.ColorBlocksTag.ColorBlock8.colorID = CreateClan.ColorBlocksName.ColorBlock8.colorID = 8;
			
			CreateClan.ColorBlocksName.ColorBlock1.ActionFunction = CreateClan.ColorBlocksName.ColorBlock2.ActionFunction = 
			CreateClan.ColorBlocksName.ColorBlock3.ActionFunction = CreateClan.ColorBlocksName.ColorBlock4.ActionFunction = 
			CreateClan.ColorBlocksName.ColorBlock5.ActionFunction = CreateClan.ColorBlocksName.ColorBlock6.ActionFunction = 
			CreateClan.ColorBlocksName.ColorBlock7.ActionFunction = CreateClan.ColorBlocksName.ColorBlock8.ActionFunction = 
			function() 
			{ 
				me.newClan_nameColor = this.colorID; 
				switch(this.colorID)
				{
					case 1:
					CreateClan.ClanName.textColor = 0xFF0000;
					break;
					case 2:
					CreateClan.ClanName.textColor = 0xFAB411;
					break;
					case 3:
					CreateClan.ClanName.textColor = 0x17F8EF;
					break;
					case 4:
					CreateClan.ClanName.textColor = 0x30F61E;
					break;
					case 5:
					CreateClan.ClanName.textColor = 0x1775F8;
					break;
					case 6:
					CreateClan.ClanName.textColor = 0xE117F8;
					break;
					case 7:
					CreateClan.ClanName.textColor = 0x666666;
					break;
					case 8:
					CreateClan.ClanName.textColor = 0xF8F8F8;
					break;
				}
			}
			CreateClan.ColorBlocksTag.ColorBlock1.ActionFunction = CreateClan.ColorBlocksTag.ColorBlock2.ActionFunction = 
			CreateClan.ColorBlocksTag.ColorBlock3.ActionFunction = CreateClan.ColorBlocksTag.ColorBlock4.ActionFunction = 
			CreateClan.ColorBlocksTag.ColorBlock5.ActionFunction = CreateClan.ColorBlocksTag.ColorBlock6.ActionFunction = 
			CreateClan.ColorBlocksTag.ColorBlock7.ActionFunction = CreateClan.ColorBlocksTag.ColorBlock8.ActionFunction = 
			function() 
			{ 
				me.newClan_tagColor = this.colorID; 
				switch(this.colorID)
				{
					case 1:
					CreateClan.ClanTag.textColor = 0xFF0000;
					break;
					case 2:
					CreateClan.ClanTag.textColor = 0xFAB411;
					break;
					case 3:
					CreateClan.ClanTag.textColor = 0x17F8EF;
					break;
					case 4:
					CreateClan.ClanTag.textColor = 0x30F61E;
					break;
					case 5:
					CreateClan.ClanTag.textColor = 0x1775F8;
					break;
					case 6:
					CreateClan.ClanTag.textColor = 0xE117F8;
					break;
					case 7:
					CreateClan.ClanTag.textColor = 0x666666;
					break;
					case 8:
					CreateClan.ClanTag.textColor = 0xF8F8F8;
					break;
				}

			}
		}
		
		public function BtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
			evt.currentTarget.parent.gotoAndPlay("pressed");
			evt.currentTarget.parent.ActionFunction();
		}

		public function BtnRollOverFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}		
		}

		public function BtnRollOverFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i=1; i<9; ++i)
				{
					var name = "ColorBlock"+i;
					if(CreateClan.ColorBlocksName[name].State == "active")
					{
						CreateClan.ColorBlocksName[name].State = "none";
						CreateClan.ColorBlocksName[name].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}		
		}
		public function BtnPressFn4(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i=1; i<9; ++i)
				{
					var name = "ColorBlock"+i;
					if(CreateClan.ColorBlocksTag[name].State == "active")
					{
						CreateClan.ColorBlocksTag[name].State = "none";
						CreateClan.ColorBlocksTag[name].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}		
		}

		private var iconsBtnArray:Array = new Array();
		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			
			CreateClan.Tab2.gotoAndPlay("pressed");
			CreateClan.Tab2.State = "active";
			CreateClan.MenuBtnClans.gotoAndPlay("pressed");
			
			CreateClan.ClanTag.textColor = 0xF8F8F8;
			CreateClan.ClanName.textColor = 0xF8F8F8;
			newClan_nameColor = 8;
			newClan_tagColor = 8;
			CreateClan.ColorBlocksName.ColorBlock8.gotoAndPlay("pressed");
			CreateClan.ColorBlocksName.ColorBlock8.State = "active";
			CreateClan.ColorBlocksTag.ColorBlock8.gotoAndPlay("pressed");
			CreateClan.ColorBlocksTag.ColorBlock8.State = "active";
			
			if (CreateClan.IconBlock.Pic.numChildren > 0)
				CreateClan.IconBlock.Pic.removeChildAt(0);
			loadSlotIcon(api.myClanIcons[newClan_currentIconID], CreateClan.IconBlock.Pic, 0);
			
			while(CreateClan.PicBrowser.PicBrowser.Pics.numChildren > 0)
				CreateClan.PicBrowser.PicBrowser.Pics.removeChildAt(0);
			
			iconsBtnArray = new Array();
			var x = -31.05;
			for(var i=0; i<api.myClanIcons.length; ++i)
			{
				var slot = new warz.frontend.ClansCreateClanBrowsePicsBlock();
				iconsBtnArray.push(slot);
				slot.iconPath = api.myClanIcons[i];
				slot.iconID = i;
				slot.x = x;
				slot.y = 0;
				
				if (slot.Pic.numChildren > 0)
					slot.Pic.removeChildAt(0);
				loadSlotIcon (slot.iconPath, slot.Pic, 1);
				
				slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event) 
										  {
											  if(evt.currentTarget.parent.State != "active")
											  	evt.currentTarget.parent.gotoAndPlay("over");
										  });
				slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event) 
										  {
											  if(evt.currentTarget.parent.State != "active")
											  	evt.currentTarget.parent.gotoAndPlay("out");
										  });
				slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:Event) 
										  {
											  if(evt.currentTarget.parent.State != "active")
											  {
												  for(var j=0; j<iconsBtnArray.length; ++j)
												  {
													  if(iconsBtnArray[j].State == "active")
													  {
														  iconsBtnArray[j].State = "none";
														  iconsBtnArray[j].gotoAndPlay("out");
													  }
												  }
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.State = "active";
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  evt.currentTarget.parent.ActionFunction();
											  }
										  });
				slot.ActionFunction = function()
				{
					newClan_selectingIconID = this.iconID;
				}
				
				CreateClan.PicBrowser.PicBrowser.Pics.addChild(slot);
				
				if(i == newClan_currentIconID)
				{
					slot.State = "active";
					slot.gotoAndPlay("over");
				}
				
				x+=216.05;
			}
			
			this.visible = true;
		}
		
		public function Deactivate()
		{
			this.visible = false;
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
			
			if(dat.slotType==0)
			{
				slotWidth = 256;
				slotHeight = 256;
				slotX = 0;
				slotY = 0;
			}
			else if(dat.slotType==1)
			{
				slotWidth = 256;
				slotHeight = 256;
				slotX = 0;
				slotY = 0;
			}
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}
	}
}