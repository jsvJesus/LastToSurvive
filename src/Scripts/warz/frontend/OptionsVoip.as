package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.utils.clearInterval;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.utils.setInterval;
	import flash.geom.Point;
	import caurina.transitions.Tweener;
	
	public class OptionsVoip extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public	var	OptionsVoipClip:MovieClip;

		public var api:warz.frontend.Frontend=null;

		public var Opt_EnableVoiceChat:Object;
		public var Opt_ShowChatBubble :Object;
		public var Opt_InputDevice :Object;
		public var Opt_OutputDevice :Object;
		public var Opt_VoiceChatVolume :Object;
	
		public var arNY :Array; // No, Yes
		
		public var arInputDevices:Array = new Array();
		public var arOutputDevices:Array = new Array();

		public	var	isMouseDown:Boolean;
		public	var	downTarget:MovieClip;

		public	function OptionsVoip ()
		{
			OptionsVoipClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.General_Options.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsVoipClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.Controls.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsVoipClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.Language.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsVoipClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.Voip.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsVoipClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.BtnReset.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsVoipClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsVoipClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsVoipClip.BtnApply.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsVoipClip.BtnReset.Text.Text.text = "$FR_Reset";
			OptionsVoipClip.BtnApply.Text.Text.text = "$FR_Apply";
			
			Opt_EnableVoiceChat = new Object();
			Opt_ShowChatBubble = new Object();
			Opt_InputDevice = new Object();
			Opt_OutputDevice = new Object();
			Opt_VoiceChatVolume = new Object();
			
			arNY = new Array("$FR_optNo", "$FR_optYes"); 
			
			var	me = this;
		}
		
		public function clearDevicesArray()
		{
			arInputDevices = new Array();
			arOutputDevices = new Array();
		}
		public function addInputDevice(name:String)
		{
			arInputDevices.push(name);
		}
		public function addOutputDevice(name:String)
		{
			arOutputDevices.push(name);
		}
		
		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			if(api.isDebug)
			{
				clearDevicesArray();
				addInputDevice("input 1");
				addInputDevice("input 2");
				addInputDevice("input 3");
				
				addOutputDevice("outpuit e8r7sify 1");
				addOutputDevice("outpuit e8r7sify 2");
				addOutputDevice("outpuit e8r7sify 3");
				addOutputDevice("outpuit e8r7sify 4");
			}
			
			this.gotoAndPlay("start");
			
			resetValues();

			OptionsVoipClip.Voip.State = "active";
			if (OptionsVoipClip.Voip.currentLable != "pressed")
				OptionsVoipClip.Voip.gotoAndPlay("pressed");

			api.setButtonText (OptionsVoipClip.General_Options.Text, "$FR_optGeneral");
			api.setButtonText (OptionsVoipClip.Controls.Text, "$FR_optControls");
			api.setButtonText (OptionsVoipClip.Language.Text, "$FR_optLanguage");
			api.setButtonText (OptionsVoipClip.Voip.Text, "$FR_optVoip");
		}
		
		public function Deactivate()
		{
			this.gotoAndPlay("end");
		}
		
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
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as OptionsVoip))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as OptionsVoip).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "General_Options")
			{
				api.showOptionsGeneral();
				
				OptionsVoipClip.General_Options.State = "off";
				OptionsVoipClip.General_Options.gotoAndPlay("out");
			}
			else if (button == "Controls")
			{
				api.showOptionsControls();
				OptionsVoipClip.Controls.State = "off";
				OptionsVoipClip.Controls.gotoAndPlay("out");
			}
			else if (button == "Language")
			{
				api.showOptionsLanguage();

				OptionsVoipClip.Language.State = "off";
				OptionsVoipClip.Language.gotoAndPlay("out");
			}
			else if (button == "Voip")
			{
//				api.showOptionsVoip();

				OptionsVoipClip.Voip.State = "off";
				OptionsVoipClip.Voip.gotoAndPlay("out");
			}
			else if (button == "BtnApply")
			{
				 
			
				OptionsVoipClip.BtnApply.State = "off";
				OptionsVoipClip.BtnApply.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ApplySettingAreYouSure", "",function(isApply:Boolean){
								if(isApply){
									Opt_VoiceChatVolume.myValue = getSliderValue(OptionsVoipClip.VoipStat4);
									FrontEndEvents.eventOptionsVoipApply(Opt_EnableVoiceChat.myValue, Opt_ShowChatBubble.myValue, 
														 Opt_InputDevice.myValue, Opt_OutputDevice.myValue, 
														 Opt_VoiceChatVolume.myValue);
								}
				});
			}
			else if (button == "BtnReset")
			{
				OptionsVoipClip.BtnReset.State = "off";
				OptionsVoipClip.BtnReset.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ResetSettingAreYouSure", "",function(isReset:Boolean){
										if(isReset)FrontEndEvents.eventOptionsVoipReset();});
			}
		}
		
		public	function resetValues ()
		{
			var	me = this;
			api = warz.frontend.Frontend.api;

			Opt_EnableVoiceChat.myValue = api.Opt_EnableVoiceChat;
			Opt_ShowChatBubble.myValue = api.Opt_ShowChatBubble;
			Opt_InputDevice.myValue = api.Opt_InputDevice;
			Opt_OutputDevice.myValue = api.Opt_OutputDevice;
			Opt_VoiceChatVolume.myValue = api.Opt_VoiceChatVolume;
			
			OptionsVoipClip.VoipStat1.Name.text = "$FR_optVoipEnable";
			OptionsVoipClip.VoipStat1.Value.Text.text = arNY[Opt_EnableVoiceChat.myValue];
			OptionsVoipClip.VoipStat1.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_EnableVoiceChat, me.arNY, this); }
			OptionsVoipClip.VoipStat1.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_EnableVoiceChat, me.arNY, this); }
			OptionsVoipClip.VoipStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat1.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsVoipClip.VoipStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat1.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsVoipClip.VoipStat2.Name.text = "$FR_optVoipShowVoiceBubble";
			OptionsVoipClip.VoipStat2.Value.Text.text = arNY[Opt_ShowChatBubble.myValue];
			OptionsVoipClip.VoipStat2.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_ShowChatBubble, me.arNY, this); }
			OptionsVoipClip.VoipStat2.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_ShowChatBubble, me.arNY, this); }
			OptionsVoipClip.VoipStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat2.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsVoipClip.VoipStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat2.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsVoipClip.VoipStat6.Name.text = "$FR_optVoipInputDevice";
			OptionsVoipClip.VoipStat6.Value.Text.text = arInputDevices[Opt_InputDevice.myValue];
			OptionsVoipClip.VoipStat6.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_InputDevice, me.arInputDevices, this); }
			OptionsVoipClip.VoipStat6.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_InputDevice, me.arInputDevices, this); }
			OptionsVoipClip.VoipStat6.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat6.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat6.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsVoipClip.VoipStat6.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat6.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat6.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsVoipClip.VoipStat7.Name.text = "$FR_optVoipOutputDevice";
			OptionsVoipClip.VoipStat7.Value.Text.text = arOutputDevices[Opt_OutputDevice.myValue];
			OptionsVoipClip.VoipStat7.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_OutputDevice, me.arOutputDevices, this); }
			OptionsVoipClip.VoipStat7.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_OutputDevice, me.arOutputDevices, this); }
			OptionsVoipClip.VoipStat7.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat7.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat7.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsVoipClip.VoipStat7.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsVoipClip.VoipStat7.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsVoipClip.VoipStat7.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsVoipClip.VoipStat4.Name.text = "$FR_optVoipVoiceChatVolume";
			OptionsVoipClip.VoipStat4.Bar.Scale.scaleX = 0.99; 
			setSliderValue(OptionsVoipClip.VoipStat4, Opt_VoiceChatVolume.myValue);
			OptionsVoipClip.VoipStat4.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			OptionsVoipClip.VoipStat4.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			OptionsVoipClip.VoipStat4.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			OptionsVoipClip.VoipStat5.visible = false;
			
			stage.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
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
				
			var myVar:Number = variable.myValue;
			var prevMode:Number = myVar;
			myVar += incr;
			if(myVar<0)
				myVar = myArray.length-1;
			if(myVar>=myArray.length)
				myVar = 0;
	
			p.Value.Value1 = myArray[prevMode];
			p.Value.Value2 = myArray[myVar];
			p.Value.Text.text = myArray[myVar];
			p.Value.gotoAndPlay ("start");
			variable.myValue = myVar;
		}
		
		private function arrowPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.ActionFunction ();
			}
		}

		private function barPressFn(evt:MouseEvent)
		{
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
	}
}
