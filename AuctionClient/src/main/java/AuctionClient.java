/* TODO: Graphical User Interface? Check if needed/optimal/good idea

*/

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
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
import java.lang.reflect.Type;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
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
    private JTextField auctionProduct;
    private JTextField auctionDescription;
    private JTextField settingsValue;
    private JTextField settingsMinimum;
    private JTextField settingsMaximum;
    private JTextField settingsTime;
    private JCheckBox settingsBidder;
    private JCheckBox settingsBidValue;
    private JCheckBox settingsAuthor;
    private JPanel auctionListTabPane;
    private JButton listMeTheStuffButton;
    private JList listAuction;
    private JButton refreshButton;
    private JComboBox comboBoxAuction;
    private JList list1;

    String ccNumber = "12345678";
    public static void main(String[] args) throws Exception {

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
            InetAddress host = InetAddress.getLocalHost();
            Socket socket = new Socket(host.getHostName(), 7777);

            ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
            oos.writeObject(jAuction);
            ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
            String message = (String) ois.readObject();
            System.out.println("Message: " + message);

            ois.close();
            oos.close();

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


                Gson gson = new Gson();
                //Default
                /*String auctionType = "Closed";
                boolean encryptedBidder = true;
                boolean encryptedBidValue = true;
                boolean encryptedAuthor = true;

                if (auctionTypeOpen.isSelected()){
                    auctionType = "Open";
                    encryptedBidder = false;
                    encryptedBidValue = false;
                    encryptedAuthor = false;
                }*/
                Settings settings = new Settings(
                        Double.parseDouble(settingsValue.getText()),
                        Double.parseDouble(settingsMinimum.getText()),
                        Double.parseDouble(settingsMaximum.getText()),
                        settingsBidder.isSelected(),
                        settingsBidValue.isSelected(),
                        Integer.parseInt(settingsTime.getText()),
                        settingsAuthor.isSelected()
                );
                String auctionSettings = gson.toJson(settings);
                Auction auction = new Auction("tbd",
                        ccNumber,
                        auctionProduct.getText(),
                        auctionDescription.getText(),
                        auctionSettings);

                String stringAuction = gson.toJson(auction);
                System.out.println(stringAuction);
                Message message = new Message("Auction", "validate", stringAuction);
                String stringMessage = gson.toJson(message);
                System.out.println(stringMessage);
                try {
                    send(stringMessage, "8500");
                }catch (Exception en) {
                }

            }
        });
        GETButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                Auction auction = new Auction();

                Gson gson = new Gson();
                String stringAuction = gson.toJson(auction);
                try {
                    ask(stringAuction);
                }catch (Exception en2) {
                }
            }
        });
        listMeTheStuffButton.addActionListener(new ActionListener() {
                        @Override
                        public void actionPerformed(ActionEvent e) {

                            Auction auction = new Auction();
                            Message message = new Message("Auction", "List", "List me the shit fam");
                            Gson gson = new Gson();
                            String stringMessage = gson.toJson(message);
                            //System.out.println(message);
                            try {
                                send(stringMessage, "8501");
                }catch (Exception en2) {
                }
            }
        });
        refreshButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                Auction auction = new Auction();
                Message message = new Message("Auction", "listInBids", "List me the shit fam");
                Gson gson = new Gson();
                String stringMessage = gson.toJson(message);
                //System.out.println(message);
                try {
                    send(stringMessage, "8501");
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

    private void send(String message2send, String port) throws Exception{


        // HTTP code
        StringEntity entity = new StringEntity(message2send,
                ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build(); //todo; change ports send(message, port)
        HttpPost request = new HttpPost("http://localhost:"+port+"/security"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE
        request.setEntity(entity);

        Gson gson = new Gson();

        HttpResponse response = httpClient.execute(request);
        Message message = gson.fromJson(message2send, Message.class);
        System.out.println(message.getAction());
        switch (message.getAction()){
            case "listInBids":
                System.out.println(";:)");

                Type type = new TypeToken<HashMap<String, ArrayList<Block>>>(){}.getType();

                String responseBody = convert(response.getEntity().getContent(),Charset.forName("UTF-8"));
                System.out.println("response:"+responseBody);
                HashMap<String, ArrayList<Block>> active_blockchains = gson.fromJson(responseBody, type);

                System.out.println(active_blockchains.size());
                for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                    String key = entry.getKey();
                    ArrayList<Block> value = entry.getValue();
                    System.out.println(value.get(0).auction.getProduct());
                    comboBoxAuction.addItem(value.get(0).auction.getProduct());
                    // ...
                }
        }
        String responseBody = EntityUtils.toString(response.getEntity());


        POSTtextpane.setText(responseBody + "\n\n" + POSTtextpane.getText()); //todo: get this out of here, into the gui part of the code



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
        HttpGet request = new HttpGet("http://localhost:8501/security/?list=all"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE



        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());
        System.out.println(responseBody);
        GETtextplane.setText(responseBody + "\n\n" + GETtextplane.getText());
    }
}



