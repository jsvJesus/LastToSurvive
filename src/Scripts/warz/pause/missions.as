package warz.pause {
	import flash.display.MovieClip;
	import warz.events.PauseEvents;
	import warz.events.SoundEvents;
	import warz.utils.ImageLoader;
	import flash.display.Bitmap;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.events.TimerEvent;
	import flash.utils.Timer;
	import warz.utils.Layout;
	import flash.geom.Point;
	import flash.geom.Rectangle;
	import flash.utils.clearInterval;
	import flash.utils.setInterval;
	import caurina.transitions.Tweener;

	public class missions extends MovieClip {
		
		public var api:warz.pause.pause=null;
		
		public var Call4Help:MovieClip;
		public var Missions:MovieClip;
		
		private var currentMissionID = 0;

		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		
		private var MissionList:Array = new Array();

		public function missions() {
			var me = this;
			
			Call4Help.Button.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Call4Help.Button.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Call4Help.Button.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Call4Help.Button.ActionFunction = function()
			{
				if(Call4Help.TextDistress.text.length < 10)
				{
					api.showInfoMsg("$HUD_CallForHelp_NoDistressMsg", true, "$ERROR");
					return;
				}
				if(Call4Help.TextReward.text.length < 3)
				{
					api.showInfoMsg("$HUD_CallForHelp_NoRewardMsg", true, "$ERROR");
					return;
				}
				PauseEvents.eventSendCallForHelp(Call4Help.TextDistress.text, Call4Help.TextReward.text);
			}
			
			Missions.DescrBlock.BtnAccept.Text.Text.text = "$FR_ACCEPT";
			Missions.DescrBlock.BtnDecline.Text.Text.text = "$FR_DECLINE";
			Missions.DescrBlock.BtnAbandon.Text.Text.text = "$FR_Abandon";
			Missions.DescrBlock.BtnActivate.Text.Text.text = "$FR_Activate";

			Missions.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Missions.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Missions.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Missions.DescrBlock.BtnAccept.ActionFunction = function()
			{
				PauseEvents.eventMissionAccept(MissionList[currentMissionID]["missionID"]);
			}
			Missions.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Missions.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Missions.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Missions.DescrBlock.BtnDecline.ActionFunction = function()
			{
				PauseEvents.eventMissionDecline(MissionList[currentMissionID]["missionID"]);
			}
			Missions.DescrBlock.BtnAbandon.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Missions.DescrBlock.BtnAbandon.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Missions.DescrBlock.BtnAbandon.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Missions.DescrBlock.BtnAbandon.ActionFunction = function()
			{
				PauseEvents.eventMissionAbandon(MissionList[currentMissionID]["missionID"]);
			}
			Missions.DescrBlock.BtnActivate.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Missions.DescrBlock.BtnActivate.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Missions.DescrBlock.BtnActivate.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			Missions.DescrBlock.BtnActivate.ActionFunction = function()
			{
			}
			
			Scroller = Missions.Scroller;
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
				var endY = me.Scroller.Field.height - me.Scroller.Trigger.height + 10;
				me.ScrollerIsDragging = true;
				me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
			})
		}
		
		public function refreshCurrentMissionInfo()
		{
			if(MissionList[currentMissionID]["isAccepted"])
			{
				Missions.DescrBlock.gotoAndStop("active");
				Missions.DescrBlock.IconText.text = "$FR_MissionActive";
				Missions.DescrBlock.Icons.gotoAndStop("accept");
			}
			else if(MissionList[currentMissionID]["isDeclined"])
			{
				Missions.DescrBlock.gotoAndStop("declined");
				Missions.DescrBlock.Icons.gotoAndStop("decline");
				Missions.DescrBlock.IconText.text = "$FR_MissionDeclined";
			}
			else
			{
				Missions.DescrBlock.gotoAndStop("regular");
				Missions.DescrBlock.IconText.text = "";
			}
			Missions.DescrBlock.Title.text = MissionList[currentMissionID]["Name"];
			Missions.DescrBlock.Text.text = MissionList[currentMissionID]["Desc"];

			while (Missions.DescrBlock.Pic.numChildren > 0)
				Missions.DescrBlock.Pic.removeChildAt(0);
			loadIcon(MissionList[currentMissionID]["icon"], Missions.DescrBlock.Pic);
				
			Missions.DescrBlock.visible = true;
		}
		
		public function loadIcon(path:String, imageHolder:MovieClip)
		{
			var	dat:Object = {imageHolder:imageHolder};
			var	imageLoader:ImageLoader = new ImageLoader (path, loadCallback, dat)
		}
		
		public	function loadCallback (bitmap:Bitmap, dat:Object)
		{
			var slotWidth=258, slotHeight=185;
			var slotX=0, slotY=0;
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.scaleX = dat.imageHolder.scaleY = 1;
			dat.imageHolder.addChild (bitmap);
		}
		
		public function MissionButtonRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel == "declined")
				return;
			if(evt.currentTarget.parent.currentLabel == "on")
				return;

			evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function MissionButtonRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel == "declined")
				return;
			if(evt.currentTarget.parent.currentLabel == "on")
				return;

			evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function MissionButtonClickFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel == "declined")
				return;
			if(evt.currentTarget.parent.currentLabel == "on")
				return;
				
			for(var i=0; i<MissionList.length; ++i)
			{
				if(MissionList[i]["movie"].currentLabel == "on")
					MissionList[i]["movie"].gotoAndPlay("pressed_out");
			}
				
			evt.currentTarget.parent.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");

			evt.currentTarget.parent.ActionFunction();
		}
		
		public function SetMissionInfo(idx:uint, isDeclined:Boolean, isAccepted:Boolean, Name:String, Desc:String, icon:String, missionID:uint)
		{
			MissionList.push({idx:idx, isDeclined:isDeclined, isAccepted:isAccepted, Name:Name, Desc:Desc, icon:icon, missionID:missionID, movie:null});
		}
		
		public	function ButtonRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function ButtonRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function ButtonPressFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");

			evt.currentTarget.parent.ActionFunction();
		}		
		
		public function clearMissionList()
		{
			MissionList = new Array();
		}
		
		public function Activate()
		{
			api = warz.pause.pause.api;
			
			Call4Help.TextDistress.text = "";
			Call4Help.TextReward.text = "";
			
			Missions.DescrBlock.visible = false;
			
			clearMissionList();
			PauseEvents.eventMissionRequestList();
			if(api.isDebug)
			{
				SetMissionInfo(0, false, false, "Mission 1", "Mission 1 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(1, true, false, "Mission 2", "Mission 2 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_AKM.png", 1);
				SetMissionInfo(2, false, true, "Mission 3", "Mission 3 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(3, false, false, "Mission 4", "Mission 4 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(4, false, false, "Mission 5", "Mission 5 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(5, false, false, "Mission 6", "Mission 6 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(6, false, false, "Mission 5", "Mission 5 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(7, false, false, "Mission 5", "Mission 5 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(8, false, false, "Mission 5", "Mission 5 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(9, false, false, "Mission 5", "Mission 5 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				SetMissionInfo(10, false, true, "Mission 3", "Mission 3 ddfhdkf dfjkhdfkj djfhd", "weapons/ASR_FAL.png", 1);
				showMissionList();
			}
			
			visible = true;
			
			PauseEvents.eventDisableHotKeys(true);
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function showMissionList()
		{
			while(Missions.List.numChildren > 0)
				Missions.List.removeChildAt(0);
				
			Missions.DescrBlock.visible = false;
				
			var startX = 43.5;
			var startY = 80.95;
			for(var i=0; i<MissionList.length; ++i)
			{
				var movie = new warz.pause.MissionsBtnMiss();
				movie.x = startX;
				movie.y = startY;
				
				MissionList[i]["movie"] = movie;
				
				movie.Text.Text.text = MissionList[i]["Name"];
				movie.Icons.visible = MissionList[i]["isDeclined"] || MissionList[i]["isAccepted"];
				if(MissionList[i]["isAccepted"])
					movie.Icons.gotoAndStop("accept");
				else if(MissionList[i]["isDeclined"])
				{
					movie.Icons.gotoAndStop("decline");
					movie.gotoAndStop("declined");
				}
				if(!MissionList[i]["isDeclined"])
					movie.gotoAndStop(1);
				
				movie.isDeclined = MissionList[i]["isDeclined"];
				movie.isAccepted = MissionList[i]["isAccepted"];
				movie.descText = MissionList[i]["Desc"];
				movie.iconPath = MissionList[i]["icon"];
				movie.missionID = MissionList[i]["missionID"];			
				
				movie.Btn.addEventListener(MouseEvent.MOUSE_OVER, MissionButtonRollOverFn);
				movie.Btn.addEventListener(MouseEvent.MOUSE_OUT, MissionButtonRollOutFn);
				movie.Btn.addEventListener(MouseEvent.CLICK, MissionButtonClickFn);
				movie.btnID = i;
				
				movie.ActionFunction = function() {
					currentMissionID = this.btnID;
					refreshCurrentMissionInfo();
				}
				
				startY += 67;
				Missions.List.addChild(movie);
			}
			
			if (MissionList.length > 6)
			{
				Scroller.visible = true;
				
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.ArrowDown.y = Missions.Mask.height - 4;
				Scroller.Field.height = (Missions.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				Missions.List.y = -0.45;
			}
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = Missions.List.height + 26;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = Missions.Mask.height - 5;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 67);
			
			if (Missions.List.y != 67 * step)
			{
				Tweener.addTween(Missions.List, {y:67 * step, time:0.25, transition:"linear"});
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

		public function Deactivate()
		{
			visible = false;
			PauseEvents.eventDisableHotKeys(false);

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
			if (e.stageX > Missions.Mask.x && 
				e.stageX < Missions.Mask.x + Missions.Mask.width + 45 &&
				e.stageY > Missions.Mask.y && 
				e.stageY < Missions.Mask.y + Missions.Mask.height)
			{
				var dist = (Missions.List.height - Missions.Mask.height) / 51;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}
}
}
