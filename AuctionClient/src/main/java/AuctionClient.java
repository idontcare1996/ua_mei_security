/* TODO: Graphical User Interface? Check if needed/optimal/good idea

*/
import java.io.*;
import java.lang.ClassNotFoundException;
import java.net.InetAddress;
import java.net.Socket;

import com.google.gson.Gson;
import com.google.gson.JsonObject;// For parsing and writing JSON
import java.sql.Timestamp;

public class AuctionClient {

    private String id;

    public static void main(String[] args) {

        Auction auction = new Auction();

        Gson gson = new Gson();
        auction.setId("Hu7uh8hihy7h8jhih");
        auction.setData("Etruscan ceramic sculpture of a boar");
        auction.setProduct("Boar Sculpture 500BC");
        Timestamp timestamp = new Timestamp(System.currentTimeMillis());
        auction.setTimestamp(timestamp);
        String jAuction = gson.toJson(auction);

        System.out.println(auction);
        System.out.println(jAuction);
        try {
            // Create a connection to the server socket on the server application
            InetAddress host = InetAddress.getLocalHost();
            Socket socket = new Socket(host.getHostName(), 7777);

            // Send a message to the client application
            ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
            OutputStreamWriter os = new OutputStreamWriter(socket.getOutputStream());
            PrintWriter pr = new PrintWriter(socket.getOutputStream());
            JsonObject json = new JsonObject();
            json.getAsJsonObject(jAuction);
            pr.print(jAuction);

            // Read and display the response message sent by server application
            ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
            String message = (String) ois.readObject();
            System.out.println("Message: " + message);

            ois.close();
            oos.close();
            os.close();
        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
        }/*
        Auction auction = createDummyObject();

        // Convert object to JSON string
        Gson gson = new Gson();
        String json = gson.toJson(auction);
        //Print the json string
        System.out.println(json);
*/
    }

    private static Auction createDummyObject() {
        Auction auction = new Auction();

        Gson gson = new Gson();
        auction.setId("Hu7uh8hihy7h8jhih");
        auction.setData("Etruscan ceramic sculpture of a boar");
        auction.setProduct("Boar Sculpture 500BC");
        Timestamp timestamp = new Timestamp(System.currentTimeMillis());
        auction.setTimestamp(timestamp);
        String jAuction = gson.toJson(auction);
        System.out.println("s");
        System.out.println(auction);
        System.out.println(jAuction);
        try {
            // Create a connection to the server socket on the server application
            InetAddress host = InetAddress.getLocalHost();
            Socket socket = new Socket(host.getHostName(), 7777);

            // Send a message to the client application
           // ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
            OutputStreamWriter os = new OutputStreamWriter(socket.getOutputStream());
            JsonObject json = new JsonObject();
            json.getAsJsonObject(jAuction);
            System.out.println(jAuction);
            os.write(jAuction);

            // Read and display the response message sent by server application
            ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
            String message = (String) ois.readObject();
            System.out.println("Message: " + message);

            ois.close();
            os.close();
            //oos.close();
        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
        }


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

