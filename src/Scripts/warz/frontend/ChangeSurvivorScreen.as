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
	
	public class ChangeSurvivorScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public  var BtnBack:MovieClip;
		public  var BtnCreate:MovieClip;
		public 	var Character:MovieClip;
		
		public 	var api:warz.frontend.Frontend=null;
		
		public	var	curHero:Hero;
		public	var	body:int;
		public	var	head:int;
		public	var	legs:int;
		
		public function ChangeSurvivorScreen() {
			this.visible = false;
			
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			BtnBack.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			BtnCreate.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			BtnCreate.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			BtnBack.Text.Text.text = "$FR_Back";
			BtnCreate.Text.Text.text = "$FR_Apply";

			Character.ArrowRightZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowRightZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowRightZoneBtn1.addEventListener(MouseEvent.CLICK, arrowPressFn);

			Character.ArrowRightZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowRightZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowRightZoneBtn2.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Character.ArrowRightZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowRightZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowRightZoneBtn3.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Character.ArrowLeftZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowLeftZoneBtn1.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowLeftZoneBtn1.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Character.ArrowLeftZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowLeftZoneBtn2.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowLeftZoneBtn2.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Character.ArrowLeftZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			Character.ArrowLeftZoneBtn3.Arrow.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			Character.ArrowLeftZoneBtn3.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			Character.TitleHead.text = "$FR_CREATE_SURVIVOR_HEAD";
			Character.TitleTorso.text = "$FR_CREATE_SURVIVOR_UPPER_BODY";
			Character.TitleLegs.text = "$FR_CREATE_SURVIVOR_LEGS";
		}

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
				
				while (!(p as ChangeSurvivorScreen))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as ChangeSurvivorScreen).ActionFunction(evt.currentTarget.parent.name);
			}
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
				
				while (!(p as ChangeSurvivorScreen))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as ChangeSurvivorScreen).ActionFunction(evt.currentTarget.name);
			}
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;

			FrontEndEvents.eventRequestPlayerRender (2);

			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			curHero = null;
			for(var j=0; j<api.HeroDB.length; ++j)
			{ 
				if(api.HeroDB[j].itemID == survivor.heroID)
				{
					curHero = api.HeroDB[j];
					break;
				}
			}					
			body = survivor.bodyID;
			head = survivor.headID;
			legs = survivor.legsID;
			UpdateArrowStatus();

			if (curHero)
			{
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
			}
			
			this.visible = true;
		}
		
		public function Deactivate()
		{
			FrontEndEvents.eventRequestPlayerRender (0);
			this.visible = false;
		}
		
		public function EventChangeOnSuccess()
		{
			api.Survivors[api.SelectedChar].bodyID = body;
			api.Survivors[api.SelectedChar].headID = head;
			api.Survivors[api.SelectedChar].legsID = legs;

			api.Main.showScreen("Survivors");
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "BtnBack")
			{
				BtnBack.State = "off";
				BtnBack.gotoAndPlay("out");

				api.Main.showScreen("Survivors");
			}
			else if (button == "BtnCreate")
			{
				BtnCreate.State = "off";
				BtnCreate.gotoAndPlay("out");

				FrontEndEvents.eventChangeOutfit(api.SelectedChar, head, body, legs);
			}
			else if (button == "ArrowLeftZoneBtn1")
			{
				Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("pressed");
				head = Math.max(0, --head);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();			
			}
			else if (button == "ArrowLeftZoneBtn2")
			{
				Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("pressed");
				body = Math.max(0, --body);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowLeftZoneBtn3")
			{
				Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("pressed");
				legs = Math.max(0, --legs);	
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowRightZoneBtn1")
			{
				Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("pressed");
				head = Math.min(++head, curHero.numHeads - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();				
			}
			else if (button == "ArrowRightZoneBtn2")
			{
				Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("pressed");
				body = Math.min(++body, curHero.numBodys - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
			else if (button == "ArrowRightZoneBtn3")
			{
				Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("pressed");
				legs = Math.min(++legs, curHero.numLegs - 1);
				FrontEndEvents.eventCreateChangeCharacter (curHero.itemID, head, body, legs);
				UpdateArrowStatus();
			}
		}
		
		public function UpdateArrowStatus()
		{
			if(head == 0)
			{
				if(Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel != "inactive")
					Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numHeads > 1 && Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_out");
			}
			if(head == curHero.numHeads-1)
			{
				if(curHero.numHeads > 1 && Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel != "inactive")
					Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_in");
			}
			if(head > 0 && head < curHero.numHeads-1)
			{
				if(Character.ArrowLeftZoneBtn1.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn1.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn1.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn1.Arrow.gotoAndPlay("inactive_out");
			}
			if(body == 0)
			{
				if(Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel != "inactive")
					Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numBodys>1 && Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_out");
			}
			if(body == curHero.numBodys-1)
			{
				if(curHero.numBodys>1 && Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel != "inactive")
					Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_in");
			}	
			if(body > 0 && body < curHero.numBodys-1)
			{
				if(Character.ArrowLeftZoneBtn2.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn2.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn2.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn2.Arrow.gotoAndPlay("inactive_out");
			}
			if(legs == 0)
			{
				if(Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel != "inactive")
					Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_in");
				if(curHero.numLegs>1 && Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_out");
			}
			if(legs == curHero.numLegs-1)
			{
				if(curHero.numLegs>1 && Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel != "inactive")
					Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_in");
			}	
			if(legs > 0 && legs < curHero.numLegs-1)
			{
				if(Character.ArrowLeftZoneBtn3.Arrow.currentFrameLabel == "inactive")
					Character.ArrowLeftZoneBtn3.Arrow.gotoAndPlay("inactive_out");
				if(Character.ArrowRightZoneBtn3.Arrow.currentFrameLabel == "inactive")
					Character.ArrowRightZoneBtn3.Arrow.gotoAndPlay("inactive_out");
			}
		}
	}
}
