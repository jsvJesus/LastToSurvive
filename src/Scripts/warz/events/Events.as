package warz.events  
{
	import flash.external.ExternalInterface;
	import warz.utils.Logger;
	
	/**
	 * Event dispatcher class responsible for sending events to the host application via the External Interface or FsCommand.
	 * There are no public methods avilable. This is an internal helper class.
	 */
	 
	public class Events
	{
		
		/**
		 * Sends the event to the host application
		 * Method accepts any number of arguments, the first paramater should be the method name
		 */
		
		protected static function send(... arguments):void
		{
			Logger.log("send: " + arguments[0] + " numArgs: " + arguments.length);
			if (ExternalInterface.available)
			{
				ExternalInterface.call.apply(null, arguments);
			}
		}	
		
		private static function fs(method:String, params:String):void
		{
			if (params != "") {
//				fscommand(method, params);
				Logger.log("FsCallback: " + method + " : " + params);
			}else {
	//			fscommand(method);
				Logger.log("FsCallback: " + method);
			}
		}	
	}
}