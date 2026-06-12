package warz.dataObjects  {
	import warz.dataObjects.*;

	public class ClanInfo  {
		
		public var myClanID:uint = 0;
		public var isAdmin:Boolean = false;
		public var myName:String = "";
		public var availableSlots:uint = 0;
		public var clanReserve:uint = 0;
		public var clanMembers:Array = new Array();
		public var logoID:uint = 0;
		
		public	function ClanInfo ():void
		{
		}
		
	}
}