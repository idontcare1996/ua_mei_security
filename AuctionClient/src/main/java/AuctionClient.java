/* TODO: Graphical User Interface? Check if needed/optimal/good idea

*/

import com.google.gson.Gson; // For parsing and writing JSON
import java.sql.Timestamp;

public class AuctionClient {

    private String id;

    public static void main(String[] args) {

        Auction auction = createDummyObject();

        // Convert object to JSON string
        Gson gson = new Gson();
        String json = gson.toJson(auction);
        //Print the json string
        System.out.println(json);

    }

    private static Auction createDummyObject() {

        Auction auction = new Auction();


        auction.setId("Hu7uh8hihy7h8jhih");
        auction.setData("Etruscan ceramic sculpture of a boar");
        auction.setProduct("Boar Sculpture 500BC");
        Timestamp timestamp = new Timestamp(System.currentTimeMillis());
        auction.setTimestamp(timestamp);

        /*
        List<String> skills = new ArrayList<>();

        skills.add("java");
        skills.add("python");
        skills.add("shell");

        staff.setSkills(skills);

        */

        return auction;



    }


}

