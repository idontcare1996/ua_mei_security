/* TODO: Graphical User Interface? Check if needed/optimal/good idea

*/

import com.google.gson.Gson; // For parsing and writing JSON
//IO stuff
import java.io.BufferedReader;
import java.io.IOException;
import java.net.ConnectException;
import java.io.InputStreamReader;

//Apache HttpClient stuff

import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.entity.ContentType;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClientBuilder;



import javax.swing.*;
import java.net.ConnectException;
import java.sql.Timestamp;

public class AuctionClient {

    private String id;
    private JTextArea testTextTextArea;
    private JPanel panel1;
    private JTabbedPane tabbedPane1;
    private JPanel auctiontabbedpane;
    private JPanel bidatabbedpane;
    private JPanel receiptstabbedpane;
    private JButton button1;

    public static void main(String[] args) throws IOException, ConnectException {

       /* Auction auction = createDummyObject();

        // Convert object to JSON string
        Gson gson = new Gson();
        String json = gson.toJson(auction);
        //Print the json string
        System.out.println(json);

        JFrame frame = new JFrame("Auction Client");

        frame.setContentPane(new AuctionClient().panel1);

        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        frame.pack();

        frame.setVisible(true);*/

        // HTTP code
        String payload = "data={" +
                "\"username\": \"admin\", " +
                "\"first_name\": \"System\", " +
                "\"last_name\": \"Administrator\"" +
                "}";
        StringEntity entity = new StringEntity(payload,
                ContentType.APPLICATION_FORM_URLENCODED);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpPost request = new HttpPost("http://localhost:8000/");
        request.setEntity(entity);

        HttpResponse response = httpClient.execute(request);
        System.out.println(response.getStatusLine().getStatusCode());
    }

    private static Auction createDummyObject() {

        Auction auction = new Auction();


        auction.setId("Hu7uh8hihy7h8jhih");
        auction.setData("Etruscan ceramic sculpture of a boar");
        auction.setProduct("Boar Sculpture 500BC");
        Timestamp timestamp = new Timestamp(System.currentTimeMillis());
        auction.setTimestamp(timestamp);

        return auction;



    }



}

