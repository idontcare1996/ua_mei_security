import java.security.MessageDigest;

public class Hasher {

    //Takes the input String, runs SHA256 on it, and outputs it.

    public static String hashSHA256(String input){

        //Try, because it's a good thing to do, and because this sometimes fails ¯\_(ツ)_/¯

        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");

            //Applies SHA256
            byte[] byteInput = digest.digest(input.getBytes("UTF-8"));

            StringBuffer hexaString = new StringBuffer(); // Hashed String stored as Hexadecimal

            for (int i = 0; i < byteInput.length; i++) {
                String hex = Integer.toHexString(0xff & byteInput[i]);
                if(hex.length() == 1) hexaString.append('0');
                hexaString.append(hex);
            }
            return hexaString.toString();
        }
        catch(Exception e) {
            throw new RuntimeException(e);
        }
    }
}