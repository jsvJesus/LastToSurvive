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

	public class SkillTreeScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var SkillTree:MovieClip;
		
		private var selectedSkillID:int = -1;
		
		public	function SkillTreeScreen ()
		{
			visible = false;
			var me = this;
			
			SkillTree.BtnLearn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			SkillTree.BtnLearn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			SkillTree.BtnLearn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			SkillTree.BtnLearn.ActionFunction = function()
			{				
				FrontEndEvents.eventLearnSkill(selectedSkillID);
			}

			SkillTree.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			SkillTree.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			SkillTree.BtnBack.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			SkillTree.BtnBack.ActionFunction = function()
			{			
				api.Main.showScreen("Survivors");
			}

			for(var i=0; i<34; ++i)
			{
				var Name = "Icon"+i;
				SkillTree[Name].SkillID = i;
				SkillTree[Name].Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
				SkillTree[Name].Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
				SkillTree[Name].Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
				SkillTree[Name].ActionFunction = function()
				{
					for(var k=0; k<34; ++k)
					{
						var n = "Icon"+k;
						if(SkillTree[n].State == "active")
						{
							SkillTree[n].State = "off";
							if(SkillTree[n].IsLearned)
								SkillTree[n].gotoAndPlay("out");
							else
								SkillTree[n].gotoAndPlay("not_learned_out");
						}
					}
					
					this.State = "active";
					selectedSkillID = this.SkillID;
					SkillTree.DescrTitle.text = api.SkillData[this.SkillID]["name"];
					
					if(this.SkillID >= 0 && this.SkillID <= 7)
						SkillTree.DescrTitle.textColor = 0xc76802;
					else if(this.SkillID >= 8 && this.SkillID <= 14)
						SkillTree.DescrTitle.textColor = 0x979797;
					else if(this.SkillID >= 15 && this.SkillID <= 18)
						SkillTree.DescrTitle.textColor = 0x446596;
					else
						SkillTree.DescrTitle.textColor = 0x3c882d;

					var	survivor:Survivor = api.Survivors[api.SelectedChar];
						
					SkillTree.DescrText.text = api.SkillData[this.SkillID]["desc"];
					SkillTree.Price.text = api.SkillData[this.SkillID]["cost"]+" XP";
					SkillTree.Price2.text = "/ "+survivor.SkillXPPool+" XP";
					
					var canLearn = true;
					if(this.SkillID > 0 && this.SkillID <= 7)
						canLearn = survivor.Skills[this.SkillID-1]>0;
					else if(this.SkillID > 8 && this.SkillID <= 14)
						canLearn = survivor.Skills[this.SkillID-1]>0;
					else if(this.SkillID > 15 && this.SkillID <= 18)
						canLearn = survivor.Skills[this.SkillID-1]>0;
					else if(this.SkillID > 19 && this.SkillID <= 30)
						canLearn = survivor.Skills[this.SkillID-1]>0;
					else if(this.SkillID >= 31)
						canLearn = false;

					if(api.SkillData[this.SkillID]["cost"]  == 0)
						canLearn = false;
						
					if(canLearn && survivor.SkillXPPool < api.SkillData[this.SkillID]["cost"])
						canLearn = false;
					
					if(this.IsLearned || !canLearn)
						SkillTree.BtnLearn.gotoAndStop("inactive");
					else
						SkillTree.BtnLearn.gotoAndStop(1);
				}
			}
			
		}
		
		public function BtnRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "inactive")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "inactive")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "inactive")
			{
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}
		}

		public function BtnRollOverFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if(evt.currentTarget.parent.IsLearned)
					evt.currentTarget.parent.gotoAndPlay("over");
				else
					evt.currentTarget.parent.gotoAndPlay("not_learned_over");				
			}
		}
		public function BtnRollOutFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if(evt.currentTarget.parent.IsLearned)
					evt.currentTarget.parent.gotoAndPlay("out");
				else
					evt.currentTarget.parent.gotoAndPlay("not_learned_out");
			}
		}
		public function BtnPressFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");
				if(evt.currentTarget.parent.IsLearned)
					evt.currentTarget.parent.gotoAndPlay("pressed");
				else
					evt.currentTarget.parent.gotoAndPlay("not_learned_pressed");
				
				evt.currentTarget.parent.ActionFunction();
			}		
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			refreshSkillTree();
			
			this.visible = true;
		}
		
		public function Deactivate()
		{
			this.visible = false;
		}
		
		public function refreshSkillTree()
		{
			var	survivor:Survivor = api.Survivors[api.SelectedChar];

			SkillTree.DescrTitle.text = "";
			SkillTree.DescrText.text = "";
			
			SkillTree.Price.text = "";
			SkillTree.Price2.text = "";
			
			selectedSkillID = -1;
			for(var k=0; k<34; ++k)
			{
				var n = "Icon"+k;
				SkillTree[n].State = "off";
				
				if (SkillTree[n].Pic.numChildren > 0)
					SkillTree[n].Pic.removeChildAt(0);
				if (SkillTree[n].PicBW.numChildren > 0)
					SkillTree[n].PicBW.removeChildAt(0);

				loadSlotIcon(api.SkillData[k]["icon"], SkillTree[n].Pic, 0);
				loadSlotIcon(api.SkillData[k]["iconBW"], SkillTree[n].PicBW, 0);
				
				SkillTree[n].IsLearned = survivor.Skills[k]>0;					
				
				if(SkillTree[n].IsLearned)
				{
					SkillTree[n].gotoAndStop(1);
				}
				else
				{
					SkillTree[n].gotoAndStop("not_learned");
				}
			}
			
			SkillTree.BtnLearn.gotoAndStop("inactive");
		}
		
		public function loadSlotIcon(path:String, imageHolder:MovieClip, slotType:Number)
		{
			var	dat:Object = {imageHolder:imageHolder, slotType:slotType};
			var	imageLoader:ImageLoader = new ImageLoader (path, loadSlotCallback, dat)
		}
		
		public	function loadSlotCallback (bitmap:Bitmap, dat:Object)
		{
			var slotWidth=128, slotHeight=128;
			var slotX=0, slotY=0;
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}
	}
}