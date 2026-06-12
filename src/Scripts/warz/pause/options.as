package warz.pause {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.geom.Point;
	import flash.geom.Rectangle;
	import flash.utils.clearInterval;
	import flash.utils.setInterval;
	import warz.events.PauseEvents;
	import warz.events.SoundEvents;
	import caurina.transitions.Tweener;

	public class options extends MovieClip {
		
		public var api:warz.pause.pause=null;
		
		public var Opt_OverallQ:Object;
		public var Opt_Brightness:Object;
		//public var Opt_Contrast:Object;
		public var Opt_SoundV:Object;
		public var Opt_MusicV:Object;
		public var Opt_CommV:Object;
		public var Opt_vertLook:Object;
		public var Opt_mouseWheel:Object;
		public var Opt_mouseSens:Object;
		public var Opt_mouseAccel:Object;

		public var arLMH :Array; // low, medium, high
		public var arLMHU :Array; // low, medium, high, ultra
		public var arLMHUC :Array; // low, medium high, ultra, custom
		public var arNY :Array; // No, Yes
		public var arLH :Array; // Low, High
		public var arOLMHU :Array; // Off, Low, Medium, High, Ultra
		public var arVLMHU :Array; // Very Low, Low, Medium, High, Ultra
		public var arOLMH :Array; // Off, Low, Medium, High

		public	var	isMouseDown:Boolean;
		public	var	downTarget:MovieClip;

		private var listenForKeyboardTarget : MovieClip;
		private var sendKeyRemapRequestInterval;
		
		public	var	keyControls:MovieClip;

		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		public function options() {
			Opt_OverallQ = new Object();
			Opt_Brightness = new Object ();
			//Opt_Contrast = new Object();
			Opt_SoundV = new Object ();
			Opt_MusicV = new Object ();
			Opt_CommV = new Object ();
			Opt_vertLook = new Object ();
			Opt_mouseWheel = new Object ();
			Opt_mouseSens = new Object ();
			Opt_mouseAccel = new Object ();

			arLMH = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig");
			arLMHU = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arLMHUC = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt", "$FR_optCus");
			arNY = new Array("$FR_optNo", "$FR_optYes"); 
			arLH = new Array("$FR_optLow", "$FR_optHig");
			arOLMHU = new Array("$FR_optOff", "$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arVLMHU = new Array("$FR_optVeryLow", "$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arOLMH = new Array("$FR_optOff", "$FR_optLow", "$FR_optMed", "$FR_optHig");

			ResetBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			ResetBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			ResetBtn.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			ResetBtn.Text.Text.text = "$FR_Reset";

			ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			ApplyBtn.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			ApplyBtn.Text.Text.text = "$FR_Apply";			
			
			keyControls = new MovieClip ();
			keyControls.mask = this.Mask;				
			this.addChild(keyControls);

			var	me = this;
			
			Scroller = KeyScroller;
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
		}
		
		public function Activate()
		{
			api = warz.pause.pause.api;

			visible = true;
			
			PauseEvents.eventDisableHotKeys(false);
			
			resetValues ();
			updateKeyMapping ();
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function Deactivate()
		{
			visible = false;
			
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
			if (e.stageX > this.Mask.x && 
				e.stageX < this.Mask.x + this.Mask.width + 45 &&
				e.stageY > this.Mask.y && 
				e.stageY < this.Mask.y + this.Mask.height)
			{
				var dist = (keyControls.height - this.Mask.height) / 51;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}
		
		public	function resetValues ()
		{
			var	me = this;
			
			Opt_OverallQ.myValue = api.Opt_OverallQ-1;
			Opt_Brightness.myValue = api.Opt_Brightness;
			//Opt_Contrast.myValue = api.Opt_Contrast;
			Opt_SoundV.myValue = api.Opt_SoundV;
			Opt_MusicV.myValue = api.Opt_MusicV;
			Opt_CommV.myValue = api.Opt_CommV;
			Opt_vertLook.myValue = api.Opt_vertLook;
			Opt_mouseWheel.myValue = api.Opt_mouseWheel;
			Opt_mouseSens.myValue = api.Opt_mouseSens;
			Opt_mouseAccel.myValue = api.Opt_mouseAccel;

// quality
//			OptStat2
			OverallQ.Name.text = "$FR_optOverallQuality";
			OverallQ.Value.Text.text = arLMHUC[Opt_OverallQ.myValue];

			OverallQ.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OverallQ.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OverallQ.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OverallQ.ArrowLeft.ActionFunction = function ()
			{
				me.onArrowClick(-1, me.Opt_OverallQ, me.arLMHUC, this);
			}			

			OverallQ.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OverallQ.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OverallQ.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OverallQ.ArrowRight.ActionFunction = function ()
			{
				me.onArrowClick(+1, me.Opt_OverallQ, me.arLMHUC, this);
			}
			
// brightness
			Brightness.Name.text = "$FR_optBrightness";
			Brightness.Bar.Scale.scaleX = 0.99; // api.Opt_Brightness;
			Brightness.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn2);
			Brightness.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn2);
			Brightness.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn2);

			setSliderValue2(Brightness, Opt_Brightness.myValue);
			
// contrast
			//Contrast.Name.text = "$FR_optContrast";
			//Contrast.Bar.Scale.scaleX = 0.99; // api.Opt_Contrast;
			//Contrast.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn2);
			//Contrast.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn2);
			//Contrast.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn2);

			//setSliderValue2(Contrast, Opt_Contrast.myValue);
			
// sound Volume
			Volume.Name.text = "$FR_optSoundVolume";
			Volume.Bar.Scale.scaleX = 0.99;
			Volume.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			Volume.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			Volume.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			setSliderValue(Volume, Opt_SoundV.myValue);

// Music Volume
			MusicVolume.Name.text = "$FR_optMusicVolume";
			MusicVolume.Bar.Scale.scaleX = 0.99; 
			MusicVolume.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			MusicVolume.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			MusicVolume.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			setSliderValue(MusicVolume, Opt_MusicV.myValue);

// Comms Volume
			SpeechVolume.Name.text = "$FR_optVoipVoiceChatVolume";
			SpeechVolume.Bar.Scale.scaleX = 0.99;
			SpeechVolume.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			SpeechVolume.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			SpeechVolume.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			setSliderValue(SpeechVolume, Opt_CommV.myValue);

// invert Y
			InvertY.Name.text = "$FR_optInvertYAxis";
			InvertY.Value.Text.text = arNY[Opt_vertLook.myValue];
			InvertY.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_vertLook, me.arNY, this); }
			InvertY.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_vertLook, me.arNY, this); }
			InvertY.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			InvertY.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			InvertY.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			InvertY.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			InvertY.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			InvertY.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			MouseWheel.Name.text = "$FR_optMouseWheel";
			MouseWheel.Value.Text.text = arNY[Opt_mouseWheel.myValue];
			MouseWheel.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_mouseWheel, me.arNY, this); }
			MouseWheel.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_mouseWheel, me.arNY, this); }
			MouseWheel.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			MouseWheel.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			MouseWheel.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			MouseWheel.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			MouseWheel.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			MouseWheel.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Sensitivity.Name.text = "$FR_optSensitivity";
			Sensitivity.Bar.Scale.scaleX = 0.99;
			Sensitivity.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn3);
			Sensitivity.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn3);
			Sensitivity.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn3);

			setSliderValue3(Sensitivity, Opt_mouseSens.myValue);
			
			Run.Name.text = "$FR_optAcceleration";
			Run.Value.Text.text = arNY[Opt_mouseAccel.myValue];
			Run.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_mouseAccel, me.arNY, this); }
			Run.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_mouseAccel, me.arNY, this); }
			Run.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Run.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Run.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			Run.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Run.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Run.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			stage.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
		}
		
		private function barPressFn(evt:MouseEvent)
		{
			SoundEvents.eventSoundPlay("menu_click");
			var	target = evt.currentTarget;
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = (coord.x / barW);

			setSliderValue (evt.currentTarget.parent, xPos);
			isMouseDown = true;
			downTarget = target;
		}
		
		private function barMoveFn(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			if (isMouseDown && downTarget  == target)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = (coord.x / barW);

				setSliderValue (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			isMouseDown = false;
			downTarget = null;
		}

		private function barPressFn2(evt:MouseEvent)
		{
			SoundEvents.eventSoundPlay("menu_click");
			var	target = evt.currentTarget;
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = 0.25 + (coord.x / barW) * 0.5;

			setSliderValue2 (evt.currentTarget.parent, xPos);
			isMouseDown = true;
			downTarget = target;
		}
		
		private function barMoveFn2(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			if (isMouseDown && downTarget  == target)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = 0.25 + (coord.x / barW) * 0.5;

				setSliderValue2 (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn2(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			isMouseDown = false;
			downTarget = null;
		}
		
		// param - from 0.0 to 1.0
		private function setSliderValue(control:MovieClip, param:Number)
		{
			param = Math.min (param, 1);
			param = Math.max (param, 0);	

			var barW:Number = 550;
			var	width:Number = param * barW;
			control.Bar.Scale.width = width;
		}
		
		private function getSliderValue(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.scaleX;
			if(ret<0) ret = 0;
			return ret;
		}

		// param - from 0.25 to 0.75
		private function setSliderValue2(control:MovieClip, param:Number)
		{
			param = (param - 0.25) / 0.5;
			param = Math.min (param, 1);
			param = Math.max (param, 0);	

			var barW:Number = 550;// control.Bar.width;
			var	width:Number = param * barW;
			control.Bar.Scale.width = width;
		}
		
		private function getSliderValue2(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.scaleX;
			
			ret = (ret * 0.5) + 0.25;
			ret = Math.min (ret, 0.75);
			ret = Math.max (ret, 0.25);	

			return ret;
		}
		
		private function barPressFn3(evt:MouseEvent)
		{
			SoundEvents.eventSoundPlay("menu_click");
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = (coord.x / barW) * 10;

			setSliderValue3 (evt.currentTarget.parent, xPos);
			isMouseDown = true;
		}
		
		private function barMoveFn3(evt:MouseEvent)
		{
			if (isMouseDown)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = (coord.x / barW) * 10;
	
				setSliderValue3 (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn3(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			isMouseDown = false;
			downTarget = null;
		}
		
		private function setSliderValue3(control:MovieClip, param:Number)
		{
			var barW:Number = 550;
			
			param = Math.min (param, 10);
			param = Math.max (param, 0);	

			var	width:Number = param / 10 * barW;
			control.Bar.Scale.width = width;
			
			var	str:String = (param).toString(10);
			if(str.length > 4)
				str = str.slice(0, 4);

//			control.Value.Text.text = str;
		}
		
		private function getSliderValue3(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.width / control.Bar.width;
			return ret*10;
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
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as options))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as options).ActionFunction(evt.currentTarget.parent.name);
			}
		}		
		
		public	function ActionFunction (button:String)
		{
			if (button == "ResetBtn")
			{
				ResetBtn.State = "off";
				ResetBtn.gotoAndPlay("out");
				api.showInfoOkCancelMsg("$FR_ResetSettingAreYouSure", "", ResetBtnCallBack); 
			}
			else if (button == "ApplyBtn")
			{
				ApplyBtn.State = "off";
				ApplyBtn.gotoAndPlay("out");	
				api.showInfoOkCancelMsg("$FR_ApplySettingAreYouSure","", ApplyBtnCallBack); 
			}
		}
		
		public function ResetBtnCallBack(isReset:Boolean):void{
			if(isReset){
				PauseEvents.eventOptionsControlsReset();
				PauseEvents.eventBackToGame();
			}
		}
		
		public function ApplyBtnCallBack(isApply:Boolean):void{
			if(isApply){
				Opt_mouseSens.myValue = getSliderValue3(Sensitivity);
				Opt_Brightness.myValue = getSliderValue2(Brightness);
				//Opt_Contrast.myValue = getSliderValue2(Contrast);
					
				Opt_SoundV.myValue = getSliderValue(Volume);
				Opt_MusicV.myValue = getSliderValue(MusicVolume);
				Opt_CommV.myValue = getSliderValue(SpeechVolume);
					
				PauseEvents.eventOptionsApply(Opt_mouseSens.myValue, Opt_Brightness.myValue, 0,//Opt_Contrast.myValue, 
											Opt_SoundV.myValue, Opt_MusicV.myValue, Opt_CommV.myValue,
											Opt_OverallQ.myValue + 1, 0, Opt_vertLook.myValue,
											Opt_mouseWheel.myValue, Opt_mouseAccel.myValue, 0); 
				PauseEvents.eventBackToGame();
			}
		}
		
		private function arrowRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		private function arrowRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		private function onArrowClick(incr:Number, variable:Object, myArray:Array, Arrow:MovieClip)
		{
			var	p:MovieClip = (Arrow.parent as MovieClip);
			if(p.State == "disabled")
				return;
				
			SoundEvents.eventSoundPlay("menu_click");
				
			var myVar:Number = variable.myValue;
			var prevMode:Number = myVar;
			myVar += incr;
			if(myVar<0)
				myVar = myArray.length-1;
			if(myVar>=myArray.length)
				myVar = 0;
	
			p.Value.Text.text = myArray[myVar];
			variable.myValue = myVar;
		}
				
		private function arrowPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.ActionFunction ();
			}
		}		
		
		public	function updateKeyMapping ()
		{
			var me = this;
			listenForKeyboardTarget = null;
//			KeyboardControls.BaseControl._visible = false;
			
			for(var i = 0; i < 8; i++)
			{
				var NewName = "Control"+ String (i + 1);
				
				if(this[NewName])
					this[NewName].visible = false;
			}
			
			while (keyControls.numChildren > 0)
			{
				keyControls.removeChildAt(0);
			}
			
			for(i=0;i<api.keyboardMapping.length;i++)
			{
				var keyControl = new warz.pause.KeyControl();
				keyControls.addChild(keyControl);
				
				keyControl.x = this["Control1"].x;
				keyControl.y = this["Control1"].y + 51 * i;
				
				keyControl.addEventListener(MouseEvent.MOUSE_OVER, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						evt.currentTarget.gotoAndPlay("over");
					}
				});
				keyControl.addEventListener(MouseEvent.MOUSE_OUT, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						evt.currentTarget.gotoAndPlay("out");
					}
				});
				keyControl.addEventListener(MouseEvent.CLICK, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						me.listenForKeyboardTarget = evt.currentTarget;
						evt.currentTarget.gotoAndPlay("pressed");
						clearInterval(me.sendKeyRemapRequestInterval);
						me.sendKeyRemapRequestInterval = setInterval (sendKeyRemapRequest, 100);
					}
				});

				keyControl.Name.Name.text = api.keyboardMapping[i]["name"];
				keyControl.Value.Text.text = api.keyboardMapping[i]["key"];
				keyControl.remapIndex = i;
			}
			
			if (api.keyboardMapping.length > 11)
			{
				Scroller.visible = true;
				
				//Scroller.Trigger.y = Scroller.Field.y;//cynthia: 53 
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);

				Scroller.ArrowDown.y = this.Mask.height - 4;
				Scroller.Field.height = (this.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				keyControls.y = 0;
			}
		}

		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = keyControls.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = this.Mask.height - 5;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 51);
			
			if (keyControls.y != 51 * step)
			{
				Tweener.addTween(keyControls, {y:51 * step, time:0.25, transition:"linear"});
			}
		
//			keyControls.y = dist * h;
		}

		public function sendKeyRemapRequest()
		{
			clearInterval(sendKeyRemapRequestInterval);
			PauseEvents.eventOptionsControlsRequestKeyRemap(listenForKeyboardTarget.remapIndex);
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
	}
}
