package warz.pause {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import warz.utils.Layout;
	import warz.utils.ImageLoader;
	import warz.dataObjects.Item;
	import flash.display.Bitmap;
	import warz.dataObjects.Survivor;
	import warz.events.PauseEvents;
	import warz.events.SoundEvents;
	
	public class changeBackpack extends MovieClip {
		public var api:warz.pause.pause=null;

		public	var	selectedBackpack:int = -1;
		
		public	function changeBackpack ()
		{
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			BtnBack.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			BtnBack.Text.Text.text = "$FR_Back";

			BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			BtnApply.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			BtnApply.Text.Text.text = "$FR_PAUSE_INVENTORY_CHANGE_BACKPACK";
			
			Title.text = "$FR_PAUSE_INVENTORY_CHANGE_BACKPACK";
			TitleHeader.text = "$FR_CURRENT_BACKPACK";
			HeaderType.text = "$FR_BACKPACK_TYPE";
			HeaderMaxWeight.text = "$FR_PAUSE_INVENTORY_MAX_WEIGHT";
			HeaderWeight.text = "$FR_CURRECT_WEIGHT";
		}
		
		public function Activate()
		{
			api = warz.pause.pause.api;

			visible = true;
			
			selectedBackpack = -1;
			
			showBackpacks ();
		}
		
		public function Deactivate()
		{
			visible = false;
			
			for (var a:int = 0; a < 10; a++)
			{
				var name = "Slot" + String (a + 1);
				
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_DOWN, backpackClicked);
			}
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
				
				while (!(p as changeBackpack))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as changeBackpack).ActionFunction(evt.currentTarget.parent.name);
			}
		}		
		
		public	function ActionFunction (button:String)
		{
			if (button == "BtnBack")
			{
				BtnBack.State = "off";
				BtnBack.gotoAndPlay("out");
				Deactivate ();
			}
			else if (button == "BtnApply")
			{
				BtnApply.State = "off";
				BtnApply.gotoAndPlay("out");
				
				if (selectedBackpack != -1)
				{	
					var	NewBP:Item = api.getItemByID(api.BackpackDB[selectedBackpack]);
					if(api.survivor.weight > NewBP.maxWeight)
						api.showInfoOkCancelMsg("$FR_PAUSE_INVENTORY_CHANGE_BACKPACK_OVERWEIGHT_WARNING", "",sureToChangeBackPack);
					else
						sureToChangeBackPack(true);
				}
			}
		}
		
		public function sureToChangeBackPack(sureToChange: Boolean):void{
				if(sureToChange){
					PauseEvents.eventChangeBackpack (selectedBackpack, api.BackpackDB[selectedBackpack]);
					Deactivate ();
				}
		}
				
		public	function showBackpacks ():void
		{
			var survivor:Survivor = api.survivor;
			var	item:Item = survivor.backpackItem;
			
			if (this.HeaderPic.numChildren > 0)
				this.HeaderPic.removeChildAt(0);
	
			loadSlotIcon (item.Icon, this.HeaderPic, 1);
			
			Type.text = String (item.maxSlots) + " $FR_SLOTS";
			MaxWeight.text = String (item.maxWeight) + " $FR_PAUSE_INVENTORY_LBS";
			Weight.text = String ((survivor.weight).toFixed(1)) + " $FR_PAUSE_INVENTORY_LBS";
			
			for (var a:int = 0; a < 10; a++)
			{
				var name = "Slot" + String (a + 1);
				
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_DOWN, backpackClicked);
			}

			for (a = 0; a < api.BackpackDB.length; a++)
			{
				var	backpackID:int = api.BackpackDB[a];
				name = "Slot" + String (a + 1);
				
				item = api.getItemByID(backpackID);
				
				if (this[name].currentFrame != 0)
					this[name].gotoAndStop (0);
				
				this[name].MaxWeight.Text.text = String (item.maxWeight);
				this[name].Type.Text.text = String (item.maxSlots);
				
				this[name].MaxWeight.visible = true;
				this[name].Type.visible = true;

				this[name].Btn.addEventListener(MouseEvent.MOUSE_OVER, backpackSlotRollOverFn);
				this[name].Btn.addEventListener(MouseEvent.MOUSE_OUT, backpackSlotRollOutFn);
				this[name].Btn.addEventListener(MouseEvent.MOUSE_DOWN, backpackClicked);
				this[name].SlotID = a;
				this[name].State = "out";

				if (this[name].Pic.numChildren > 0)
					this[name].Pic.removeChildAt(0);
		
				loadSlotIcon (item.Icon, this[name].Pic, 1);
			}
			
			for (; a < 10; a++)
			{
				name = "Slot" + String (a + 1);
				
				if (this[name].currentLabel != "locked")
					this[name].gotoAndPlay ("locked");

				this[name].MaxWeight.visible = false;
				this[name].Type.visible = false;
			}
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
			
			if (dat.slotType == 1)
			{
				slotWidth = 128;
				slotHeight = 128;
				slotX = 64;
				slotY = 64;
			}

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x += slotX; 
			bitmap.y += slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}		
		
		public	function backpackSlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.gotoAndPlay("over");
			}
		}
		
		public	function backpackSlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.gotoAndPlay("out");
			}
		}
		
		public	function backpackClicked(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for (var a:int = 0; a < api.BackpackDB.length; a++)
				{
					var	name = "Slot" + String (a + 1);
					
					if (this[name].State == "active")
					{
						this[name].State = "out";
						this[name].gotoAndPlay("out");
					}
				}
			
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.State = "active";
				selectedBackpack = evt.currentTarget.parent.SlotID;				
			}
		}		
	}
}