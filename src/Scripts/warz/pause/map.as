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
	import flash.geom.Matrix;
	
	import fl.motion.MatrixTransformer;

	public class map extends MovieClip {
		
		public var api:warz.pause.pause=null;
		
		public	var	hasPlayerPosition:Boolean;
		public	var	MapFrame:MovieClip;
		public	var	Position:MovieClip;
		
		public	var	playerX:Number;
		public	var	playerY:Number;
		public	var	playerRotation:Number;
		
		public var MapLayer:MovieClip;
		
		public var GroupPlayers:Array = new Array();
		public var ClanPlayers:Array = new Array();
		public var CallForHelpEvents:Array = new Array();
		public var MissionIcons:Array = new Array(); // warz.pause.MapMissionsIcons, MissionIconsList
		
		var scaleFactor:Number = 0.8;
		var minScale:Number = 1.0;
		var maxScale:Number = 1.8;
		
		public function map() {
			
			MapZoom.MinusBtn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			MapZoom.MinusBtn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			MapZoom.MinusBtn.addEventListener(MouseEvent.CLICK, ButtonPressFn);

			MapZoom.PlusBtn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			MapZoom.PlusBtn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			MapZoom.PlusBtn.addEventListener(MouseEvent.CLICK, ButtonPressFn);

			MapZoom.MinusBtn.ActionFunction = function()
			{
				ZoomFn(true);
			}
			MapZoom.PlusBtn.ActionFunction = function()
			{
				ZoomFn(false);
			}
			
			MapFrame.mouseEnabled = false;
			
		}
		
		private function ZoomFn(op:Boolean)
		{
			var mat:Matrix;
			
			var externalCenter:Point=new Point(983, 516);
			var internalCenter:Point=MapLayer.globalToLocal(new Point(983, 516));
			
			if (op) {
				MapLayer.scaleX = Math.max(scaleFactor*MapLayer.scaleX, minScale);
				MapLayer.scaleY = Math.max(scaleFactor*MapLayer.scaleY, minScale);
			}
			else {
				MapLayer.scaleX = Math.min(1/scaleFactor*MapLayer.scaleX, maxScale);
				MapLayer.scaleY = Math.min(1/scaleFactor*MapLayer.scaleY, maxScale);
			}
			
			mat = MapLayer.transform.matrix.clone();
			MatrixTransformer.matchInternalPointWithExternal(mat,internalCenter,externalCenter);
			MapLayer.transform.matrix=mat;
		}
		
		public	var debugTimer:Timer;

		public function Activate()
		{
			api = warz.pause.pause.api;
			visible = true;
			
			hasPlayerPosition = false;			
			
			GroupPlayers = new Array();
			ClanPlayers = new Array();
			CallForHelpEvents = new Array();
			MissionIcons = new Array();
			
			PauseEvents.eventShowMap ();
			if(api.isDebug)
			{
				addGroupPlayer(0.5, 0.5);
				addClanPlayer(0.24, 0.25);
				addCallForHelpEvent(0.6, 0.7, "dfjhd djkfhdjfh djfh", "dfy dskjfh sdfkjh", "sagor");
				addMissionIcon(0.1, 0.5, "kill");
				addMissionIcon(0.2, 0.5, "kill");
				addMissionIcon(0.3, 0.5, "kill");
				submitPlayerLocations();
			}
			
			stage.addEventListener(MouseEvent.MOUSE_DOWN, startDragging);
			stage.addEventListener(MouseEvent.MOUSE_UP, stopDragging);
			MapLayer.addEventListener(Event.ENTER_FRAME, pictureFrame);
			
			PauseEvents.eventDisableHotKeys(false);
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_DOWN, startDragging);
			stage.removeEventListener(MouseEvent.MOUSE_UP, stopDragging);
			MapLayer.removeEventListener(Event.ENTER_FRAME, pictureFrame);
			visible = false;
		}
		
		public function addGroupPlayer(posx:Number, posy:Number)
		{
			GroupPlayers.push({posx:posx, posy:posy, movie:null});
		}
		public function addClanPlayer(posx:Number, posy:Number)
		{
			ClanPlayers.push({posx:posx, posy:posy, movie:null});
		}
		public function addCallForHelpEvent(posx:Number, posy:Number, distressText:String, rewardText:String, username:String)
		{
			CallForHelpEvents.push({posx:posx, posy:posy, distressText:distressText, rewardText:rewardText, username:username, movie:null});
		}
		public function addMissionIcon(posx:Number, posy:Number, iconType:String)
		{
			MissionIcons.push({posx:posx, posy:posy, iconType:iconType, movie:null});
		}
		public function submitPlayerLocations()
		{
			while(ClanPlayersList.numChildren > 0)
				ClanPlayersList.removeChildAt(0);

			for(var i=0; i<ClanPlayers.length; ++i)
			{
				var slot = new warz.pause.MapClanPointer();
				ClanPlayers[i]["movie"] = slot;

				slot.x = 0;
				slot.y = 0;
				
				ClanPlayersList.addChild(slot);
			}

			while(GroupPlayersList.numChildren > 0)
				GroupPlayersList.removeChildAt(0);

			for(i=0; i<GroupPlayers.length; ++i)
			{
				slot = new warz.pause.MapGroupPointer();
				GroupPlayers[i]["movie"] = slot;

				slot.x = 0;
				slot.y = 0;
				
				GroupPlayersList.addChild(slot);
			}
			
			while(CallForHelpList.numChildren > 0)
				CallForHelpList.removeChildAt(0);
				
			for(i=0; i<CallForHelpEvents.length; ++i)
			{
				slot = new warz.pause.MapCallForHelp();
				CallForHelpEvents[i]["movie"] = slot;

				slot.x = 0;
				slot.y = 0;
				slot.Popup.TextDistress.text = CallForHelpEvents[i]["distressText"];
				slot.Popup.TextReward.text = CallForHelpEvents[i]["rewardText"];
				slot.Username.text = CallForHelpEvents[i]["username"];
				slot.Popup.visible = false;
				
				
				slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event) {
										  evt.currentTarget.parent.Popup.visible = true;
										  });
				slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event) {
										  evt.currentTarget.parent.Popup.visible = false;
										  });
				
				CallForHelpList.addChild(slot);
			}

			while(MissionIconsList.numChildren > 0)
				MissionIconsList.removeChildAt(0);
			for(i=0; i<MissionIcons.length; ++i)
			{
				slot = new warz.pause.MapMissionsIcons();
				MissionIcons[i]["movie"] = slot;

				slot.x = 0;
				slot.y = 0;
				slot.gotoAndStop(MissionIcons[i]["iconType"]);
				MissionIconsList.addChild(slot);
			}
		}
		
		function startDragging(e:MouseEvent):void {
			if (e.stageX > Mask.x && 
				e.stageX < Mask.x + Mask.width &&
				e.stageY > Mask.y && 
				e.stageY < Mask.y + Mask.height)
				MapLayer.startDrag();
		}
		function stopDragging(mev:MouseEvent):void {
			MapLayer.stopDrag();
		}
		function pictureFrame(ev:Event):void {
			if(MapLayer.x > 540)
				MapLayer.x = 540;
			if((MapLayer.x+MapLayer.width) < (540+885))
				MapLayer.x = (540+885)-(MapLayer.width);

			if(MapLayer.y > 97)
				MapLayer.y = 97;
			if((MapLayer.y+MapLayer.height) < (97+885))
				MapLayer.y = (97+885)-(MapLayer.height);

			Pointer.x = MapLayer.x + MapLayer.width * playerX;
			Pointer.y = MapLayer.y + MapLayer.height * playerY;
			Pointer.rotation = playerRotation;
			
			for(var i=0; i<ClanPlayers.length; ++i)
			{
				ClanPlayers[i]["movie"].x = MapLayer.x + MapLayer.width * ClanPlayers[i]["posx"];
				ClanPlayers[i]["movie"].y = MapLayer.y + MapLayer.height * ClanPlayers[i]["posy"];
			}
			for(i=0; i<GroupPlayers.length; ++i)
			{
				GroupPlayers[i]["movie"].x = MapLayer.x + MapLayer.width * GroupPlayers[i]["posx"];
				GroupPlayers[i]["movie"].y = MapLayer.y + MapLayer.height * GroupPlayers[i]["posy"];
			}
			for(i=0; i<CallForHelpEvents.length; ++i)
			{
				CallForHelpEvents[i]["movie"].x = MapLayer.x + MapLayer.width * CallForHelpEvents[i]["posx"];
				CallForHelpEvents[i]["movie"].y = MapLayer.y + MapLayer.height * CallForHelpEvents[i]["posy"];
			}
			for(i=0; i<MissionIcons.length; ++i)
			{
				MissionIcons[i]["movie"].x = MapLayer.x + MapLayer.width * MissionIcons[i]["posx"];
				MissionIcons[i]["movie"].y = MapLayer.y + MapLayer.height * MissionIcons[i]["posy"];
			}
		}
		
		public	function setMap (icon:String)
		{
			var	imageLoader:ImageLoader = new ImageLoader (icon, loadMapCallback, null)
		}
		
		public	function setPlayerPosition (x:Number, y:Number, angle:Number)
		{
			playerX = x;
			playerY = y;
			playerRotation = angle;
		}
		
		public	function loadMapCallback (bitmap:Bitmap, dat:*)
		{
			while (MapLayer.numChildren > 0)
				MapLayer.removeChildAt(0);
				
			bitmap = Layout.stretch (bitmap, 885, 885, bitmap.width, bitmap.height, "exact");
			bitmap.x = 0;
			bitmap.y = 0;
			
			MapLayer.addChild (bitmap);
		}
		
		public	function ButtonRollOverFn(evt:Event)
		{
			evt.currentTarget.gotoAndPlay("over");
		}
		
		public	function ButtonRollOutFn(evt:Event)
		{
			evt.currentTarget.gotoAndPlay("out");
		}
		
		public	function ButtonPressFn(evt:Event)
		{
			evt.currentTarget.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");

			evt.currentTarget.ActionFunction();
		}		
	}
}
