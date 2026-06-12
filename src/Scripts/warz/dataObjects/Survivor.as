package warz.dataObjects  {
	public class Survivor  {
		public	var	Name:String;
		public	var	health:int;
		
		public	var	level:int;
		public	var	xp:int;
		
		public	var	hardcore:int;
		public 	var heroID:int;
		public	var	headID:int;
		public	var	bodyID:int;
		public	var	legsID:int;
		
		public	var	alive:int;
		public	var	hunger:int;
		public	var	thirst:int;
		public	var	toxic:int;
		
		public	var	timeplayed:int;
		public	var	weight:Number;
		
		private	var	backpack:Backpack;
		public	var	backpackItem:Item;
		
		public	var	zombiesKilled:int;
		public	var	banditsKilled:int;
		public	var	civiliansKilled:int;
		public	var	alignment:String;
		public	var	lastMap:String;
		
		public var globalInventory:Boolean;
		
		public var Skills:Array;
		
		public var SkillXPPool:int;
		
		public	function Survivor (Name:String, health:Number, xp:int, timeplayed:int,
									hardcore:int, heroID:int, headID:int, bodyID:int, legsID:int,
									alive:int, hunger:int, thirst:int, toxic:int, weight:Number, 
									zombiesKilled:int, banditsKilled:int, civiliansKilled:int,
									alignment:String, lastMap:String, globalInventory:Boolean,
									skillXPPool:int)
		{
			this.Name = Name;
			this.health = health;
			this.xp = xp;
			this.timeplayed = timeplayed;
			
			this.hardcore = hardcore;
			
			this.heroID = heroID;
			this.headID = headID;
			this.bodyID = bodyID;
			this.legsID = legsID;
			
			this.alive = alive;
			this.hunger = hunger;
			this.thirst = thirst;
			this.toxic = toxic;
			this.weight = weight;
			
			this.zombiesKilled = zombiesKilled;
			this.banditsKilled = banditsKilled;
			this.civiliansKilled = civiliansKilled;
			this.alignment = alignment;
			this.lastMap = lastMap;
			
			this.globalInventory = globalInventory;
			this.SkillXPPool = skillXPPool;
			
			Skills = new Array();
			for(var i=0; i<34; ++i)
				Skills.push(0);			
		}
		
		public	function setBackpack (backpack:Item, size:int):void
		{
			backpackItem = backpack;
			this.backpack = new Backpack (size);
		}
		
		public	function getBackpackSize ():int
		{
			return backpack.backpackSize;
		}
		
		public	function getBackpackName ():String
		{
			return backpackItem.Name;
		}
		
		public	function getBackpackMaxWeight():Number
		{
			return backpackItem.maxWeight;
		}
		
		public	function addBackpackItem (slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, description:String = ""):void
		{
			backpack.addItem (slotID, new BackpackItem (slotID, inventoryID, itemID, quantity, var1, var2, false, description));
		}
		
		public	function getBackpackItem (slotID:int):BackpackItem
		{
			return backpack.getBackpackItem (slotID);
		}
		
		public	function findBackpackItem (itemID:uint):BackpackItem
		{
			return backpack.findBackpackItem(itemID);
		}
		
		public	function clearBackpack ():void
		{
			backpack.clearBackpack();
		}
		
		public function removeBackpackItem (slotID:int):void
		{
			backpack.removeBackpackItem(slotID);
		}		 
		
		public	function swapBackpackSlots(slot1:int, slot2:int):void
		{
			backpack.swapBackpackSlots (slot1, slot2);
		}
		
		public	function getStatus ():String
		{
			var	statusStr:String = "Healthy";
			
/*			if (health > 0 && health < 100)
				statusStr = "Wounded";
			else if (health == 0)
				statusStr = "Dead";
*/
			statusStr = String (int (health)) + "%";
	
			return statusStr;
		}
		
		public	function getLevel ():int
		{
			return xp;
		}
	}
}