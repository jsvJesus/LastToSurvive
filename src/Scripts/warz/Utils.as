package warz {
	
	public class Utils {

		public function Utils() {
			// constructor code
		}
		
		static public function randRange(minNum:Number, maxNum:Number):Number 
        {
            return (Math.floor(Math.random() * (maxNum - minNum + 1)) + minNum);
        }
		
		static public function trimWhitespace(str:String):String {
			if (str == null) {
				return "";
			}
			return str.replace(/^\s+|\s+$/g, "");
		}
	}
	
}
