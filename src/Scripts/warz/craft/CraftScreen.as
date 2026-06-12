package warz.craft {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import warz.events.CraftEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	import scaleform.gfx.MouseEventEx;
	import caurina.transitions.Tweener;
	import flash.geom.Point;
	
	public class CraftScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.craft.craft=null;

		public var RecipeList:MovieClip;
		public var DescrBlock:MovieClip;
		public var BtnBack:MovieClip;
		public var BtnCraft:MovieClip;

		private var ScrollerIsDragging:Boolean;
		public	var	Scroller:MovieClip;
		
		public var currentRecipeID:uint = 0;

		public function CraftScreen() {
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			BtnBack.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			BtnBack.Text.Text.text = "$HUD_Pause_ReturnToGame";

			BtnCraft.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			BtnCraft.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			BtnCraft.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			BtnCraft.Text.Text.text = "$FR_CraftItem";

			var me = this;

			Scroller = RecipeList.Scroller;
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

			this.setChildIndex(this.MsgBox, this.numChildren-1);
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			var	dist:Number;
			var	h;
			
			var stagePoint:Point = new flash.geom.Point(e.stageX, e.stageY);
			var localPoint:Point = RecipeList.globalToLocal(stagePoint);
			
			if (localPoint.x > RecipeList.Mask.x && 
				localPoint.x < RecipeList.Mask.x + RecipeList.Mask.width + 45 &&
				localPoint.y > RecipeList.Mask.y && 
				localPoint.y < RecipeList.Mask.y + RecipeList.Mask.height)
			{
				dist = (RecipeList.List.height - RecipeList.Mask.height) / 43;
				h = Scroller.Field.height - Scroller.Trigger.height + 26;
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
				var endY = Scroller.Field.height - Scroller.Trigger.height+25;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}

		// event functions
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
			if (evt is MouseEventEx)
			{
				if ((evt as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
				{
					return;
				}
			}
			
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");

				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as CraftScreen))
				{
					p = p.parent as MovieClip;
				}
				
				(p as CraftScreen).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function Activate()
		{
			this.mouseEnabled = true;
			
			refreshRecipeList();
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
		}
		
		public function Deactivate()
		{
			this.mouseEnabled = false;

			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
		}

		public function ActionFunction (button:String)
		{
			if (button == "BtnBack")
			{
				BtnBack.State = "off";
				BtnBack.gotoAndPlay("out");

				CraftEvents.eventReturnToGame();
			}
			else if (button == "BtnCraft")
			{
				BtnCraft.State = "off";
				BtnCraft.gotoAndPlay("out");

				CraftEvents.eventCraftItem(currentRecipeID);
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
			
			if (dat.slotType == 1)
			{
				slotWidth = 256;
				slotHeight = 256;
			}
			else if (dat.slotType == 2)
			{
				slotWidth = 111;
				slotHeight = 111;
			}

			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = 0; 
			bitmap.y = 0; 
			
			if(dat.slotType == 1) // center it
			{
				bitmap.x = (256-bitmap.width)/2;
				bitmap.y = (256-bitmap.height)/2;
			}
			else if(dat.slotType == 2) // center it
			{
				bitmap.x = (111-bitmap.width)/2;
				bitmap.y = (111-bitmap.height)/2;
			}
		
			dat.imageHolder.scaleX = dat.imageHolder.scaleY = 1;
			dat.imageHolder.addChild (bitmap);
		}
		
		public function refreshRecipeList()
		{
			// clear previous recipes
			while(RecipeList.List.numChildren > 0)
				RecipeList.List.removeChildAt(0);
				
			currentRecipeID = 0;
			BtnCraft.visible = false;
				
			var startY = 1;
			var slotHeight = 43;
			for(var i=0; i<api.RecipeDB.length; ++i)
			{
				var slot = new warz.craft.RecipeBtn();
				slot.x = 0;
				slot.y = startY;
				slot.Text.Text.text = api.RecipeDB[i]["name"];
				slot.recipeID = api.RecipeDB[i]["recipeID"];
				
				slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event) {
										  if(evt.currentTarget.parent.currentLabel == "on")
										  	return;
										  if(evt.currentTarget.parent.currentLabel == "declined")
										  	return;
										  evt.currentTarget.parent.gotoAndPlay("over");
										  });
				slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event) {
										  if(evt.currentTarget.parent.currentLabel == "on")
										  	return;
										  if(evt.currentTarget.parent.currentLabel == "declined")
										  	return;
										  evt.currentTarget.parent.gotoAndPlay("out");
										  });
				slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:Event) {
										  if(evt.currentTarget.parent.currentLabel == "on")
										  	return;
										  if(evt.currentTarget.parent.currentLabel == "declined")
										  	return;
										  
										  for(var k=0; k<RecipeList.List.numChildren; ++k)
										  {
											  var s = RecipeList.List.getChildAt(k);
											  if(s.currentLabel == "on")
											  	s.gotoAndPlay("pressed_out");
										  }
										  
										  SoundEvents.eventSoundPlay("menu_click");
										  evt.currentTarget.parent.gotoAndPlay("pressed");
										  currentRecipeID = evt.currentTarget.parent.recipeID;
										  showRecipeInfo(currentRecipeID);
										  });
				
				startY += slotHeight;
				RecipeList.List.addChild(slot);
			}
			
			if(api.RecipeDB.length>6)
			{
				Scroller.visible = true;
				var me = this;
				
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = RecipeList.Mask.height - 4;
				Scroller.Field.height = (RecipeList.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				RecipeList.List.y = 0;
				
				for(i=api.RecipeDB.length+1; i<=6; ++i)
				{
					slot = new warz.craft.RecipeBtn();
					slot.x = 0;
					slot.y = startY;
					slot.gotoAndPlay("empty");
					
					startY += slotHeight;
					RecipeList.List.addChild(slot);
				}
			}			
			
			DescrBlock.visible = false;
			
			if(api.RecipeDB.length==0)
			{
				api.showInfoMsg("$Craft_noRecipes", true);
			}
		}
		
		public function showRecipeInfo(recipeID:uint)
		{
			var recipe = null;
			var components = null;
			for(var i=0; i<api.RecipeDB.length; ++i)
			{
				if(api.RecipeDB[i]["recipeID"]==recipeID)
				{
					recipe = api.RecipeDB[i];
					components = api.RecipeDB[i]["components"];
					break;
				}
			}
			
			DescrBlock.RecipeTitle.text = recipe["name"];
			DescrBlock.DescrText.text = recipe["desc"];
			
			var	survivor:Survivor = api.survivor;
			
			for(i=0; i<components.length; ++i)
			{
				n = "Component"+(i+1);
				var itemID = components[i]["compID"];
				var q = components[i]["quantity"];
				var item:Item = api.getItemByID(itemID);
				
				// check if we have enough items in backpack
				var itemQinB = 0;
				for(var k=0; k<survivor.getBackpackSize(); ++k)
				{
					var bi:BackpackItem = survivor.getBackpackItem(k);
					if(bi)
					{
						if(bi.itemID == itemID)
							itemQinB += bi.quantity;
					}
				}
				if(itemID == 301386)
					itemQinB = api.ResourceMetal;
				else if(itemID == 301387)
					itemQinB = api.ResourceStone;
				else if(itemID == 301388)
					itemQinB = api.ResourceWood;
				
				DescrBlock[n].NumText.Num.text = String(q);
				while(DescrBlock[n].Pic.numChildren > 0)
					DescrBlock[n].Pic.removeChildAt(0);
				loadSlotIcon(item.Icon, DescrBlock[n].Pic, 2);				

				if(itemQinB>=q)
				{
					DescrBlock[n].gotoAndStop("regular");
					DescrBlock[n].Pic.alpha=1;
				}
				else
				{
					DescrBlock[n].gotoAndStop("insufficient");
					DescrBlock[n].Pic.alpha=0.4;
				}
			}
			// remove all other icons
			for(i=components.length+1; i<=10; ++i)
			{
				var n = "Component"+i;
				DescrBlock[n].gotoAndStop("empty");
				DescrBlock[n].Pic.alpha = 0;
			}

			while(DescrBlock.Pic.numChildren > 0)
				DescrBlock.Pic.removeChildAt(0);
			loadSlotIcon(recipe["icon"], DescrBlock.Pic, 1);
						
			DescrBlock.visible = true;
			BtnCraft.visible = true;
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = RecipeList.List.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = RecipeList.Mask.height;
			h -= mh;
			h /= h1;
		
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 43);
			
			if (RecipeList.List.y != 43 * step)
			{
				Tweener.addTween(RecipeList.List, {y:43 * step, time:api.tweenDelay, transition:"linear"});
			}
		}
	}
}
