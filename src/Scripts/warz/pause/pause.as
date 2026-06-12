package warz.pause {
	import flash.display.MovieClip;
	import caurina.transitions.Tweener;
	import warz.dataObjects.Backpack;
	import warz.dataObjects.Survivor;
	import warz.dataObjects.Item;
	import warz.dataObjects.InventoryItem;
	import warz.dataObjects.BackpackItem;
	import flash.events.MouseEvent;
	import warz.events.PauseEvents;
	import scaleform.gfx.Extensions;
	
	public class pause extends MovieClip {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;

		static public var api:pause = null;
		
		public	var		CategoryDB:Array;
		public	var		ItemDB:Array;
		public	var		SlotDB:Array;
		public	var		BackpackDB:Array;
		
		public	var		survivor:Survivor;
		
		public var m_Language:String;
		public var m_DisableLanguageSelection:Boolean = false;

		public	var	keyboardMapping:Array = null;
		
		public	var	Opt_ScreenResolutions:Array = null;
		public var Opt_ScreenResIndex :Number;
		public var Opt_OverallQ :Number;
		public var Opt_Brightness :Number;
		public var Opt_Contrast :Number;
		public var Opt_SoundV :Number;
		public var Opt_MusicV :Number;
		public var Opt_CommV :Number;
		public var Opt_hintS :Number;
		public var Opt_RedBlood :Number;
		public var Opt_AA :Number;
		public var Opt_VSync :Number;
		public var Opt_Fullscreen :Number;
		public var Opt_SSAO :Number;
		public var Opt_TerrainD :Number;
		public var Opt_TerrainT :Number;
		public var Opt_WaterQ :Number;
		public var Opt_ShadowQ :Number;
		public var Opt_LightQ :Number;
		public var Opt_EffectsQ :Number;
		public var Opt_MeshD :Number;
		public var Opt_Aniso :Number;
		public var Opt_PostProc :Number;
		public var Opt_MeshT :Number;
	
		public var Opt_vertLook :Number;
		public var Opt_leftSideCam :Number;
		public var Opt_mouseWheel :Number;
		public var Opt_mouseSens :Number;
		public var Opt_mouseAccel :Number;
		public var Opt_toggleAim :Number;
		public var Opt_toggleCrouch :Number;		
		
		public	var	language:int = 0;		

		public function pause(main:MovieClip)
		{
			api = this;
			Main = main;
			
			scaleform.gfx.Extensions.enabled = true;
			
			Tweener.init();
			
			CategoryDB = new Array ();
			ItemDB = new Array ();
			SlotDB = new Array ([], [], [], [], []);
			BackpackDB = new Array ();
			
			Opt_ScreenResolutions = new Array ();
			keyboardMapping = new Array();
		}
		
		public function setServerName(name:String):void
		{
			Main.MainMenu.MainMenu.Time.ServerName.text = name;
			Main.MainMenu.MainMenu.Time.ServerNameShad.text = name;
		}
		
		public function setResourcesNum(numWood:uint, numStone:uint, numMetal:uint)
		{
			Main.Inventory.Resources.NumWood.text = numWood;
			Main.Inventory.Resources.NumStone.text = numStone;
			Main.Inventory.Resources.NumMetal.text = numMetal;
		}
		
		public	function setTime (date:String, time:String):void
		{
			Main.MainMenu.MainMenu.Time.Date.text = date;
			Main.MainMenu.MainMenu.Time.DateShad.text = date;
			Main.MainMenu.MainMenu.Time.Time.text = time;
			Main.MainMenu.MainMenu.Time.TimeShad.text = time;
		}
		
		public function addCategory(catID:Number, name:String, tabID:int, slotID:int, slotID2:int=-1):void
		{
			var	cat:Object = {catID:catID, name:name}; 
			
			if (slotID != -1)
				SlotDB[slotID].push (cat);			
			if (slotID2 != -1)
				SlotDB[slotID2].push (cat);			
			
			CategoryDB.push (cat);
		}
		
		public	function findItemSlot (catID:Number):Array
		{
			var res:Array = new Array();
			for (var a:int = 0; a < 5; a++)
			{
				var	items:Array = SlotDB[a];
				
				for (var s:int = 0; s < items.length; s++)
				{
					var	item = items[s];
					
					if (item["catID"] == catID)
					{
						res.push(a);
					}
				}
			}
			if(res.length==0)
				res.push(-1);
			return res;
		}

		public function addItem(itemID:uint, catID:Number, name:String, desc:String, icon:String, 
								isStackable:Boolean = false, weight:Number = 0, maxSlots:int = 1, maxWeight:Number = 0):void
		{
			name = name.toUpperCase();
			ItemDB.push(new Item (itemID, catID, name, desc, icon, new Array(), isStackable, weight, maxSlots, maxWeight));
		}
		
		public	function updateClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize)
		{
			if (survivor.Name == Name)
			{
				survivor.health = health;
				survivor.xp = xp;
				survivor.timeplayed = timeplayed;
				survivor.hunger = hunger;
				survivor.alive = alive;
				survivor.thirst = thirst;
				survivor.toxic = toxic;
				
				if (survivor.backpackItem.itemID != backpackID)
				{
					var	backpack:Item = getItemByID(backpackID);
					
					survivor.setBackpack (backpack, backpackSize);
				}
			}
		}
		
		public function updateClientSurvivorWeight(Name:String, weight:Number)
		{
				if (survivor.Name == Name)
				{
					survivor.weight = weight;
				}
		}
		
		public	function addClientSurvivor (Name:String, health:Number, xp:int, timeplayed:int,
											hardcore:int, heroID:int, headID:int, bodyID:int, legsID:int,
											alive:int, hunger:int, thirst:int, toxic:int,
											backpackID:int, backpackSize:int, weight:Number,
											zombiesKilled:int, banditsKilled:int, civiliansKilled:int,
											alignment:String, lastMap:String, globalInventory:Boolean)
		{
			survivor = new Survivor (Name, health, xp, timeplayed, hardcore, heroID, headID, bodyID, legsID, 
									  alive, hunger, thirst, toxic, weight, 
									  zombiesKilled, banditsKilled, civiliansKilled, alignment, lastMap, globalInventory, 0);
												  
			var	backpack:Item = getItemByID(backpackID);
			
			if (backpack)
			{
				survivor.setBackpack (backpack, backpackSize);
			}
		}
		
		public function getItemByID(itemID:uint):Item
		{
			var item:Item;
			
			for (var a = 0; a < ItemDB.length; a++)
			{
				var	i:Item = ItemDB[a];
				if (i.itemID == itemID)
				{
					item = ItemDB[a];
				}
			}
			
			if(item == null)
			{
				item = new Item (itemID, 0, "$FR_EmptySlot", "", "", new Array(), false, 0, 0, 0);
				return item;
			}
			
			return item;
		}
		
		public	function showInventory (state:Boolean):void
		{
			Main.showInventory (state);
		}
		
		public	function showMap (state:Boolean):void
		{
			Main.showMap (state);
		}
		
		public	function showMissions (state:Boolean):void
		{
			Main.showMissions (state);
		}
		
		public	function showOptions (state:Boolean):void
		{
			Main.showOptions (state);
		}
		
		public	function backpackGridSwapSuccess ()
		{
			Main.Inventory.showCurItem ();
			Main.Inventory.showBackpack ();
			Main.Inventory.showDescription ();
		}
		
		public	function addBackpackItem (slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, description:String = "")
		{
			var	backpackItem:BackpackItem = survivor.getBackpackItem (slotID);
			
			if (backpackItem)
			{
				backpackItem.quantity += quantity;
				backpackItem.Description = description;
			}
			else		
				survivor.addBackpackItem (slotID, inventoryID, itemID, quantity, var1, var2, description);
		}
		
		public function showInfoOkCancelMsg(msg:String, title:String = "", callback:* = null)
		{
			Main.showInfoOkCancelMsg (msg, title, callback);
		}
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			Main.showInfoMsg (msg, OKButton, title);
		}
		public function hideInfoMsg()
		{
			Main.MsgBox.visible = false; 
		}

		public function showInfoMsg2(msg:String, OKButtonText:String, title:String = "")
		{
			Main.showInfoMsg2 (msg, OKButtonText, title);
		}
		
		public function clearBackpack()
		{
			survivor.clearBackpack ();
		}
		
		public function setBackpackName()
		{
			Main.Inventory.setBackpackName (survivor.getBackpackName());
		}
		
		public function setMaxWeight()
		{
			Main.Inventory.setMaxWeight (survivor.getBackpackMaxWeight ());
		}
		
		public	function setMapIcon (icon:String):void
		{
			Main.Map.setMap (icon);
		}
		
		public	function setPlayerPosition (x:Number, y:Number, angle:Number):void
		{
			Main.Map.setPlayerPosition (x, y, angle);
		}
		
		public function clearScreenResolutions()
		{
			Opt_ScreenResolutions = new Array();
		}
		
		public function addScreenResolution(res:String)
		{
			Opt_ScreenResolutions.push(res);
		}
		
		public function setOptions(res:Number, ovQ:Number, brightness:Number, contrast:Number, soundV:Number, musicV:Number, commV:Number, hintS:Number, redBlood:Number, 
					AA:Number, ssao:Number, terrD:Number, terrT:Number, waterQ:Number, shadowQ:Number, lightQ:Number, effectQ:Number,
					meshD:Number, aniso:Number, postproc:Number, meshT:Number,
					vertLook:Number, leftSideCam:Number, mouseWheel:Number, MouseSens:Number, MouseAccel:Number, toggleAim:Number, toggleCrouch:Number,
					fullscreen:Number, vsync:Number)
		{
			Opt_ScreenResIndex=res;
			Opt_OverallQ=ovQ;
			Opt_Brightness=brightness;
			Opt_Contrast=contrast;
			Opt_SoundV=soundV;
			Opt_MusicV=musicV;
			Opt_CommV=commV;
			Opt_hintS=hintS;
			Opt_RedBlood=redBlood;
			Opt_AA=AA;
			Opt_VSync=vsync;
			Opt_SSAO=ssao;
			Opt_TerrainD=terrD;
			Opt_TerrainT=terrT;
			Opt_WaterQ=waterQ;
			Opt_ShadowQ=shadowQ;
			Opt_LightQ=lightQ;
			Opt_EffectsQ=effectQ;
			Opt_MeshD=meshD;
			Opt_Aniso=aniso;
			Opt_PostProc=postproc;
			Opt_MeshT=meshT;
	
			Opt_vertLook=vertLook;
			Opt_leftSideCam=leftSideCam;
			Opt_mouseWheel=mouseWheel;
			Opt_mouseSens=MouseSens;
			Opt_mouseAccel=MouseAccel;
			Opt_toggleAim=toggleAim;
			Opt_toggleCrouch=toggleCrouch;
			
			Opt_Fullscreen = fullscreen;
		}
		
		public function addKeyboardMapping(name:String, key:String)
		{
			keyboardMapping.push({name:name, key:key});
		}
	
		public function setKeyboardMapping(name:String, key:String)
		{
			for(var i=0; i<keyboardMapping.length; ++i)
			{
				if(keyboardMapping[i]["name"]==name)
				{
					keyboardMapping[i]["key"] = key;
					break;
				}
			}
		}
	
		public function updateKeyboardMapping(index:Number, key:String)
		{
			keyboardMapping[index]["key"] = key;
			Main.Options.updateKeyMapping();
		}
		
		public function setLanguage(lang:String):void
		{
			m_Language = lang;
		}
	
		public function disableLanguageSelection()
		{
			m_DisableLanguageSelection = true;
		}
		
		public	function clearBackpacks ():void
		{
			BackpackDB = new Array ();
		}
		
		public	function addBackpack (slotID:int, itemID:uint):void
		{
			BackpackDB[slotID] = itemID;
		}
		
		public	function changeBackpackSuccess ()
		{
			Main.Inventory.showBackpack ();
		}

		public function reloadOptions()
		{
			Main.Options.resetValues();
			Main.Options.updateKeyMapping ();
		}		
	}
}