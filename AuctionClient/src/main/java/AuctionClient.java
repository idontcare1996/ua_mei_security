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
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Type;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
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
    private JEditorPane editorPane1;
    private JButton refreshButton;
    private JComboBox comboBoxAuction;
    private JTextField textFieldDescription;
    private JEditorPane editorPane2;
    private JButton bidButton;
    private JTextField bidValue;

    String ccNumber = "12345678";
    Integer RepositoryPort = 8501;
    Integer ManagerPort = 8500;

    private static HashMap<String, ArrayList<Block>> active_blockchains = new HashMap<>();
    private static HashMap<String, ArrayList<Block>> terminatedBlockchain = new HashMap<>();



    public static void main(String[] args) throws Exception {

        //Start the GUI
        JFrame frame = new JFrame("Auction Client");
        frame.setContentPane(new AuctionClient().panel1);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);

        //Grab the latest repositories from the AuctionRepository



    }


    public AuctionClient() {
        POSTButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                Gson gson = new Gson();

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
                    String result = postMessage(stringMessage, ManagerPort);
                    System.out.println(result);
                    editorPane1.setText(editorPane1.getText() + "\n\n" + result);
                } catch (Exception en) {
                }

            }
        });

        listMeTheStuffButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                String stringMessage = "";

                try {
                    stringMessage = (getMessage("open",RepositoryPort));
                } catch (Exception en2) {
                }
                Gson gson = new Gson();
                Type type = new TypeToken<HashMap<String, ArrayList<Block>>>(){}.getType();

                System.out.println("response:"+stringMessage);
                HashMap<String, ArrayList<Block>> active_blockchains = gson.fromJson(stringMessage, type);

                System.out.println(active_blockchains.size());
                editorPane2.setText("");
                for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                    String key = entry.getKey();
                    ArrayList<Block> value = entry.getValue();
                    System.out.println(value.get(0).auction.getProduct());
                    String seller = value.get(0).auction.getSeller();
                    String product = value.get(0).auction.getProduct();
                    String uuid = key;
                    editorPane2.setText(editorPane2.getText() + "\n\n" + "Seller: "+seller+"  Product: "+product+"\n    UUID:<"+uuid+">");
                    // ...
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
                    //Copies the response from the POST to a string
                    String auctionstring = getMessage("open", RepositoryPort);
                    System.out.println(auctionstring);

                    //Sets a type, to use in the gson.fromJson
                    Type type = new TypeToken<HashMap<String, ArrayList<Block>>>(){}.getType();

                    //refreshes the client's active_blockchain.


                    active_blockchains = gson.fromJson(auctionstring, type);
                    comboBoxAuction.removeAllItems();
                    for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                        String key = entry.getKey();
                        ArrayList<Block> value = entry.getValue();
                        String product = (value.get(0).auction.getProduct());
                        String uuid = (value.get(0).auction.getId());
                        comboBoxAuction.addItem(product+"   <"+uuid+">");
                    }


                } catch (Exception en2) {
                }
            }
        });
        comboBoxAuction.addItemListener(new ItemListener() {
            @Override
            public void itemStateChanged(ItemEvent e) {
                String chosen_auction = comboBoxAuction.getSelectedItem().toString();
                String uuid = (chosen_auction.substring(chosen_auction.indexOf("<")+1));
                uuid = uuid.substring(0, uuid.length() - 1);
                textFieldDescription.setText(uuid);

            }
        });
        bidButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                    Gson gson = new Gson();
                    System.out.println(textFieldDescription.getText());
                    Bid bid = new Bid(textFieldDescription.getText(),
                            Long.parseLong(bidValue.getText()),
                            "Author");//todo: from
                    String auctionBid = gson.toJson(bid);


                    Message message = new Message("Bid", "Add", auctionBid);
                    String stringMessage = gson.toJson(message);
                    System.out.println(stringMessage);
                    try {
                        String result = postMessage(stringMessage, RepositoryPort);
                        System.out.println(result);
                        //editorPane1.setText(editorPane1.getText() + "\n\n" + result);
                    } catch (Exception en) {
                    }

                }

        });

    }



    public static String convert(InputStream inputStream, Charset charset) throws IOException {

        try (Scanner scanner = new Scanner(inputStream, charset.name())) {
            return scanner.useDelimiter("\\A").next();
        }
    }

    private String postMessage(String mensage, Integer port) throws Exception {


        // HTTP code
        StringEntity entity = new StringEntity(mensage, ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpPost request = new HttpPost("http://localhost:" + port + "/security");
        request.setEntity(entity);

        Gson gson = new Gson();

        HttpResponse response = httpClient.execute(request);
        //Message message = gson.fromJson(mensage, Message.class);
        //System.out.println(message.getAction());
        /*switch (message.getAction()){
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
        }*/
        String responseBody = EntityUtils.toString(response.getEntity());
        return responseBody;
    }

    private String getMessage(String query, Integer port) throws Exception {

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpGet request = new HttpGet("http://localhost:" + port + "/security?=" + query);

        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());
        System.out.println(responseBody);
        return responseBody;
    }

}



