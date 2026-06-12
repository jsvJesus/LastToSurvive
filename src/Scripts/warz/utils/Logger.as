package warz.utils
{
	public	class Logger
	{
		
		public static var enabled:Boolean = false;
		
		public static function log(value)
		{
			write(value);
		}
		
		public static function warn(value)
		{
			write("WARN: " + value);
		}
		
		public static function error(value)
		{
			write("ERROR: " + value);
		}
		
		public static function object(value:Object)
		{
			// recursive list
		}
		
		public static function array(value:Array)
		{
			// recursive list
		}
		
		private static function write(value)
		{
			if (enabled) trace(value);
		}
	}
}