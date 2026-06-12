package warz.hud {
	import flash.display.MovieClip;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import warz.events.HUDEvents;
	import flash.events.KeyboardEvent;
	public class main extends MovieClip {
		
		public	var api:warz.hud.HUD = null;
		
		public	var	Msg:MovieClip;

		public function Main() {
		}
		
		public	function init ():void
		{
			api = warz.hud.HUD.api;
			
			Msg.Msg.visible = false;
			Slots.gotoAndPlay ("out");
			
			if (!Chat.TextWindows)
			{
				Chat.TextWindows = new Array ();
				
				for (var a = 0; a < 4; a++)
				{
					var clip = new MovieClip ();
					clip.x = 12;
					clip.y = 12;
					Chat.Chat.addChild(clip);
					
					Chat.TextWindows[a] = clip;
					
					clip.Chats = new Array ();
				}
			}
			
			NotesWrite.visible = false;
			NotesWrite.NotesWrite.Text.mouseWheelEnabled = true;
			NotesWrite.NotesWrite.BtnBackPost.Text.Text.text = "$FR_Back";
			NotesWrite.NotesWrite.BtnBackPost.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			NotesWrite.NotesWrite.BtnBackPost.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			NotesWrite.NotesWrite.BtnBackPost.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);

			NotesWrite.NotesWrite.BtnPost.Text.Text.text = "$FR_Post";
			NotesWrite.NotesWrite.BtnPost.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			NotesWrite.NotesWrite.BtnPost.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			NotesWrite.NotesWrite.BtnPost.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			
			NotesRead.visible = false;
			NotesRead.NoteRead.Text.mouseWheelEnabled = true;
			NotesRead.NoteRead.Popup.visible = false;
			NotesRead.NoteRead.BtnClose.Text.Text.text = "$FR_Close";
			NotesRead.NoteRead.BtnClose.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			NotesRead.NoteRead.BtnClose.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			NotesRead.NoteRead.BtnClose.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);

			NotesRead.NoteRead.BtnReport.Text.Text.text = "$FR_ReportAbuse";
			NotesRead.NoteRead.BtnReport.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			NotesRead.NoteRead.BtnReport.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			NotesRead.NoteRead.BtnReport.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			
			Grave.visible = false;
			Grave.BtnGraveClose.Text.Text.text = "$FR_Close";
			Grave.BtnGraveClose.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Grave.BtnGraveClose.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Grave.BtnGraveClose.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			
			Safelock.visible = false;
			Safelock.LockBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			Safelock.LockBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			Safelock.LockBtn.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
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
				
			var	p:MovieClip = evt.currentTarget.parent;
				
			while (!(p as main))
			{
				p = p.parent as MovieClip;
			}
				
			(p as main).ActionFunction(evt.currentTarget.parent.name);
		}		
		
		public function showSafelock(isUnlock:Boolean, isShow:Boolean)
		{
			Safelock.Title.text = isUnlock?"$FR_Safelock_Unlocktext":"$FR_Safelock_LockText";
			Safelock.TextNum.text = "";
			Safelock.TextNum.displayAsPassword = isUnlock;
			Safelock.LockBtn.Text.Text.text = isUnlock?"$FR_Safelock_Unlock":"$FR_Safelock_Lock";
			Safelock.LockBtn.Icon.gotoAndStop(isUnlock?"open":"lock");
			Safelock.visible = isShow;
		}
		
		public	function ActionFunction (button:String)
		{
			if (button == null)
				return;
			else if (button == "BtnBackPost")
			{
				HUDEvents.eventNoteClosed();
				api.showNoteWrite(false);
			}
			else if (button == "BtnPost")
			{
				HUDEvents.eventNoteWritePost(NotesWrite.NotesWrite.Text.text);
				api.showNoteWrite(false);
			}
			else if (button == "BtnClose")
			{
				HUDEvents.eventNoteClosed();
				api.showNoteRead(false, "");
			}
			else if (button == "BtnReport")
			{
				HUDEvents.eventNoteReportAbuse();
				NotesRead.NoteRead.Popup.visible = true;
				//api.showNoteRead(false, "");
			}		
			else if( button=="BtnGraveClose")
			{
				HUDEvents.eventGraveNoteClosed();
				api.showGraveNote(false, "", "", "");
			}
			else if (button == "LockBtn")
			{
				if(Safelock.TextNum.text != "")
				{
					HUDEvents.eventSafelockPass(Safelock.TextNum.text);
					Safelock.visible = false;
				}
			}
		}
		
		public	var debugTimer:Timer;
		public	var msgTimer:Timer;

		public	function startDebugTimer ():void
		{
			debugTimer = new Timer (100);
			debugTimer.addEventListener(TimerEvent.TIMER, debugTimerHandler);
			debugTimer.start(); 
			
			msgTimer = new Timer (5000);
			msgTimer.addEventListener(TimerEvent.TIMER, msgTimerHandler);
			msgTimer.start(); 
			
			visibility = 0;
			
			food = Math.random() * 100;
			water = Math.random() * 100;
			health = Math.random() * 100;
			toxicity = Math.random() * 100;
			stamina = Math.random() * 100;

			api.showChat (true, true);
			api.showSlots (true);
			api.setSlotCooldown(0, 105, 0);
			api.setSlotCooldown(1, 105, 0);
			api.setSlotCooldown(2, 30, 30);

			
			api.setChatTransparency (0.25);
		}
		
		public	var	visibility:Number;
		
		public	var	food:Number;
		public	var	water:Number;
		public	var	health:Number;
		public	var	toxicity:Number;
		public	var	stamina:Number;
		
		public	var	msgNum:int;
		public	var	showChat:Boolean;

		public function msgTimerHandler (evt:Event)
		{
			api.showMsg("Msg " + String (++msgNum));
			
			showChat = !showChat;
			api.showSlots (true);
			
			var	text:String = "Lorem Ipsum is simply dummy text of the printing and typesetting.";
			var	user:String = "FILHARVEY";
			
			var	extra:int = int (Math.random() * 3);
			
			while (extra > 0)
			{
				text += " Lorem Ipsum is simply dummy text of the printing and typesetting.";
				extra--;
			}
			
			api.receiveChat (user + ": " + text);
			
			api.setActivatedSlot (Math.random() * 6);
//			api.setActiveSlot (Math.random() * 6);
		}
		
		public function debugTimerHandler (evt:Event)
		{
			visibility += 1;
			if (visibility > 100)
				visibility = 1;
				
			food += Math.random();
			
			if (food > 100)
			{
				food = 0;
			}
			
			water += Math.random();
			
			if (water > 100)
			{
				water = 0;
			}
			
			health += Math.random();
			
			if (health > 100)
			{
				health = 0;
			}
			
			toxicity += Math.random();
			
			if (toxicity > 100)
			{
				toxicity = 0;
			}
			
			stamina += Math.random();
			
			if (stamina > 100)
			{
				stamina = 100;
			}
			
//			health = 100;
//			toxicity = 100;

			//api.setThreatValue(visibility);
			
			api.setHeroCondition (food, water, health, toxicity, stamina);
			
			/*var temp = api.addBanditTag();
			api.moveUserIcon(temp, Math.random()*800, Math.random()*800, true);
			api.removeUserIcon(temp);*/
			
			api.setSlotCooldown(0, 105, 0);
			api.setSlotCooldown(1, 105, 0);
			api.setSlotCooldown(2, 30, 30);
			//api.setActivatedSlot (Math.random()%5);

		}
	}
}
