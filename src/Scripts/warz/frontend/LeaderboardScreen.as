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

	public class LeaderboardScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var Leadrboards:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		
		//private var currentTypeBoard:int = 0;
		private var currentBoardSelect:int = 0;
		
		private var leaderboardData:Array = new Array();

		public	function LeaderboardScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = Leadrboards.Scroller;
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
			
			/*Leadrboards.TabBig1.Text.Text.text = "$FR_NormalMode";
			Leadrboards.TabBig2.Text.Text.text = "$FR_HardcoreMode";
			Leadrboards.TabBig1.Text.Icon.gotoAndStop("norm");
			Leadrboards.TabBig2.Text.Icon.gotoAndStop("hard");
			
			Leadrboards.TabBig1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Leadrboards.TabBig1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Leadrboards.TabBig1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Leadrboards.TabBig1.ActionFunction = function() 
			{
				currentTypeBoard = 0;
				updateLeaderboard();
			}

			Leadrboards.TabBig2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Leadrboards.TabBig2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Leadrboards.TabBig2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Leadrboards.TabBig2.ActionFunction = function() 
			{
				currentTypeBoard = 1;
				updateLeaderboard();
			}*/

			Leadrboards.MenuBtnClans.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.MenuBtnClans.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.MenuBtnClans.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.MenuBtnClans.ActionFunction = function() 
			{
				if(api.isTrialAccount)
				{
					api.showTrialAccountLimit("$TrialAccountLimit_ClansNotAvailable", "$TrialAccountLimit");
				}
				else
					api.Main.showScreen("Clans");
			}
			Leadrboards.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.MenuBtnFriends.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.MenuBtnFriends.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			Leadrboards.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.MenuBtnNews.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.MenuBtnNews.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			
			Leadrboards.MenuBtnFriends.visible = false;
			Leadrboards.MenuBtnNews.visible = false;
			
			Leadrboards.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.BtnNavLeft.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.BtnNavLeft.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestLeaderboardData(/*currentTypeBoard*/0, currentBoardSelect, 2);
			}
			Leadrboards.BtnNavTop.Text.Text.text = "$FR_Top";
			Leadrboards.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.BtnNavTop.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.BtnNavTop.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestLeaderboardData(/*currentTypeBoard*/0, currentBoardSelect, 1);
			}
			Leadrboards.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Leadrboards.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Leadrboards.BtnNavRight.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Leadrboards.BtnNavRight.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestLeaderboardData(/*currentTypeBoard*/0, currentBoardSelect, 3);
			}
			
			Leadrboards.TabSmall1.Text.Text.text = "$FR_LB_XP";
			Leadrboards.TabSmall2.Text.Text.text = "$FR_LB_TimePlayed";
			Leadrboards.TabSmall3.Text.Text.text = "$FR_LB_KZ";
			Leadrboards.TabSmall4.Text.Text.text = "$FR_LB_KS";
			Leadrboards.TabSmall5.Text.Text.text = "$FR_LB_KB";
			Leadrboards.TabSmall6.Text.Text.text = "$FR_LB_HEROES";
			Leadrboards.TabSmall7.Text.Text.text = "$FR_LB_BANDITS";
			Leadrboards.TabSmall1.TabID = 0;
			Leadrboards.TabSmall2.TabID = 1;
			Leadrboards.TabSmall3.TabID = 2;
			Leadrboards.TabSmall4.TabID = 3;
			Leadrboards.TabSmall5.TabID = 4;
			Leadrboards.TabSmall6.TabID = 5;
			Leadrboards.TabSmall7.TabID = 6;
			
			Leadrboards.TabSmall1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall4.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall5.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall6.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall6.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall6.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Leadrboards.TabSmall7.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Leadrboards.TabSmall7.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Leadrboards.TabSmall7.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
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

		/*public function BtnRollOverFn2(evt:Event)
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
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.ActionFunction();
				if(evt.currentTarget.parent == Leadrboards.TabBig1)
				{
					Leadrboards.TabBig2.State = "none";
					Leadrboards.TabBig2.gotoAndPlay("out");
				}
				else
				{
					Leadrboards.TabBig1.State = "none";
					Leadrboards.TabBig1.gotoAndPlay("out");
				}
			}		
		}*/

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
				for(var i =1; i<=7; ++i)
				{
					var n = "TabSmall"+i;
					if(Leadrboards[n].State == "active")
					{
						Leadrboards[n].State = "none";
						Leadrboards[n].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				currentBoardSelect = evt.currentTarget.parent.TabID;
				updateLeaderboard();
			}
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			if (!(survivor))
			{
				var	text:String = "$FR_PLAY_GAME_NO_SURVIVORS";
				
				api.showInfoMsg (text, true, "$FR_PlayGame");
				api.Main.MainMenu.CommunityBtn.State = "off";
				api.Main.MainMenu.CommunityBtn.gotoAndPlay("out");

				api.Main.MainMenu.SurvivorsBtn.Btn.dispatchEvent(new Event(MouseEvent.CLICK));
				return;
			}
			
			/*Leadrboards.TabBig1.gotoAndPlay("pressed");
			Leadrboards.TabBig1.State = "active";
			Leadrboards.TabBig2.gotoAndStop(1);
			Leadrboards.TabBig2.State = "none";*/
			Leadrboards.MenuBtnLeader.gotoAndPlay("pressed");
			
			for(var i =1; i<=7; ++i)
			{
				var n = "TabSmall"+i;
				if(Leadrboards[n].State == "active")
				{
					Leadrboards[n].State = "none";
					Leadrboards[n].gotoAndStop(1);
				}
			}
			
			Leadrboards.TabSmall1.gotoAndPlay("pressed");
			Leadrboards.TabSmall1.State = "active";
			
			//currentTypeBoard = 0;
			currentBoardSelect = 0;
			
			this.visible = true;
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);

			updateLeaderboard();
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = false;
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}

		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > Leadrboards.Mask.x && 
				e.stageX < Leadrboards.Mask.x + Leadrboards.Mask.width + 45 &&
				e.stageY > Leadrboards.Mask.y && 
				e.stageY < Leadrboards.Mask.y + Leadrboards.Mask.height)
			{
				var dist = (Leadrboards.Slots.height - Leadrboards.Mask.height) / 48;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
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


		public function updateLeaderboard()
		{
			FrontEndEvents.eventRequestLeaderboardData(/*currentTypeBoard*/0, currentBoardSelect, 0);
			if(api.isDebug)
			{
				clearLeaderboardList();
				addLeaderboardData(0, "first player", true, "10000");
				for(var i=0; i<50; ++i)
					addLeaderboardData(i+1, "player", false, "10000");
				addLeaderboardData(100, "last player", true, "10000");
				
				setLeaderboardText("pos   NAME                                                      status               XP          info      add");
				
				populateLeaderboard();
			}
		}
		
		public function clearLeaderboardList()
		{
			leaderboardData = new Array();
		}
		
		public function addLeaderboardData(pos:uint, name:String, Alive:Boolean, data:String)
		{
			leaderboardData.push({pos:pos, name:name, Alive:Alive, data:data, movie:null});
		}
		
		public function setLeaderboardText(txt:String)
		{
			Leadrboards.ListTop.Text.text = txt;
		}
		
		public function populateLeaderboard()
		{
			while (Leadrboards.Slots.numChildren > 0)
			{
				Leadrboards.Slots.removeChildAt(0);
			}			
			
			var y = 19;
			for(var i=0; i<leaderboardData.length; ++i)
			{
				var slot = new warz.frontend.LeadrboardsSlot();
				leaderboardData[i]["movie"] = slot;
				slot.ldrData = leaderboardData[i];
				slot.x = 0;
				slot.y = y;
				slot.Text.Pos.text = leaderboardData[i]["pos"];
				slot.Text.Name.htmlText = leaderboardData[i]["name"];
				slot.Text.Num.text = leaderboardData[i]["data"];
				if(leaderboardData[i]["Alive"])
					slot.Icon.gotoAndStop("alive");
				else
					slot.Icon.gotoAndStop("dead");
				
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
												  for(var j=0; j<leaderboardData.length; ++j)
												  {
													  if(leaderboardData[j]["movie"].State == "active")
													  {
														  leaderboardData[j]["movie"].State = "none";
														  leaderboardData[j]["movie"].gotoAndPlay("out");
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
				}
				
				Leadrboards.Slots.addChild(slot);
				
				y+=48;
			}
			
			if(leaderboardData.length > 13)
			{
				Scroller.visible = true;
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = Leadrboards.Mask.height - 4;
				Scroller.Field.height = (Leadrboards.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				Leadrboards.Slots.y = 317;
			}		
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = Leadrboards.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = Leadrboards.Mask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 48);
			
			if (Leadrboards.Slots.y != (317+48*step))
			{
				Tweener.addTween(Leadrboards.Slots, {y:317+(48*step), time:api.tweenDelay, transition:"linear"});
			}
		}
		
	}
}