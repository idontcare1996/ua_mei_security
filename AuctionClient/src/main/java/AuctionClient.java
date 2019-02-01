/* TODO: Graphical User Interface? Check if needed/optimal/good idea

*/

import com.google.gson.Gson;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.util.EntityUtils;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.io.InputStream;
import java.net.ConnectException;
import java.nio.charset.Charset;
import java.util.Scanner;

//IO stuff
//Apache HttpClient stuff

public class AuctionClient {

    private String id;
    private JTextArea testTextTextArea;
    private JPanel panel1;
    private JTabbedPane tabbedPane1;
    private JPanel auctiontabbedpane;
    private JPanel bidatabbedpane;
    private JPanel receiptstabbedpane;
    private JButton POSTButton;
    private JTextPane POSTtextpane;
    private JButton GETButton;
    private JTextPane GETtextplane;
    private JPanel PostJPanel;
    private JPanel GETJpanel;


    public static void main(String[] args) throws IOException, ConnectException,Exception {

    //Start the GUI
        JFrame frame = new JFrame("Auction Client");
        frame.setContentPane(new AuctionClient().panel1);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);

    // Testing the creation of messages in JSON format

        // Create new auction
        // public Auction(String id, String type, String seller,String product, String settings)


        // Create the auction message with the created auction
        // public JsonObject AuctionMessage(String id, String author, String action, Auction auction)




        /*
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
    public AuctionClient() {
        POSTButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                Auction auction = new Auction("id000001",
                        "eng",
                        "me,myself and I",
                        "Etruscan ceramic sculpture of a boar",
                        "random");

                Gson gson = new Gson();
                String stringauction = gson.toJson(auction);

                try {
                    send(stringauction);
                }catch (Exception en) {
                }

            }
        });
        GETButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                Auction auction = new Auction("id000001",
                        "eng",
                        "me,myself and I",
                        "Etruscan ceramic sculpture of a boar",
                        "random");

                Gson gson = new Gson();
                String stringauction = gson.toJson(auction);

                try {
                    ask(stringauction);
                }catch (Exception en2) {
                }
            }
        });
    }


    // Create a new auction

    /*
    private String AuctionMessage(String id, String author, String type,String action, String data)
    {
        JsonObject auctionmessage = new JsonObject();
        // Put stuff inside json
        return auctionmessage;
    }

    private String AuctionMessage(String id, String author, String action, Bid bid)
    {
        JsonObject bidmessage = new JsonObject();

        return bidmessage;
    }
    */

    public static String convert(InputStream inputStream, Charset charset) throws IOException {

        try (Scanner scanner = new Scanner(inputStream, charset.name())) {
            return scanner.useDelimiter("\\A").next();
        }
    }

    private void send(String message2send) throws Exception{


        // HTTP code
        StringEntity entity = new StringEntity(message2send,
                ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpPost request = new HttpPost("http://localhost:8500/example"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE
        request.setEntity(entity);


        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());

        POSTtextpane.setText(responseBody + "\n\n" + POSTtextpane.getText());


            /*
            InetSocketAddress hostAddress = new InetSocketAddress("localhost", 8090);
            SocketChannel client = SocketChannel.open(hostAddress);

            System.out.println("Sending...");




                byte [] message = new String(message2send).getBytes();
                ByteBuffer buffer = ByteBuffer.wrap(message);
                client.write(buffer);
                System.out.println(message2send);
                buffer.clear();
                Thread.sleep(50);
        System.out.println("Sent...");
            client.close();
            */
    }
    private void ask(String what) throws Exception{
        // HTTP code
        StringEntity entity = new StringEntity(what, ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpGet request = new HttpGet("http://localhost:8500/example"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE



        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());

        GETtextplane.setText(responseBody + "\n\n" + GETtextplane.getText());
    }
}



