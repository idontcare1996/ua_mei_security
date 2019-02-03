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
import pt.gov.cartaodecidadao.*;

import javax.swing.*;
import java.awt.event.*;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Type;
import java.nio.charset.Charset;
import java.security.AccessController;
import java.security.PrivilegedAction;
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
    private JButton openUpdateButton;
    private JEditorPane editorPane1;
    private JButton refreshButton;
    private JComboBox comboBoxAuction;
    private JTextField textFieldUUID;
    private JEditorPane editorPaneOpenAuctionsList;
    private JButton bidButton;
    private JTextField bidValue;
    private JEditorPane editorPane3;
    private JButton closedUpdateButton;
    private JEditorPane editorPaneClosedAuctionsList;
    private JComboBox comboBoxTerminate;
    private JButton terminateButton;

    private static String ccNumber = "12345678";
    private static String ccName = "";
    private static String ccKey;
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

        //Identification using Citizenship Card
        // create configuration
        System.setProperty("java.library.path", "D:\\UNIVERSIDADE\\Security\\ua_mei_security\\AuctionClient");
        loadPteidLib();

        PTEID_ReaderSet.initSDK();
        PTEID_EIDCard card;
        PTEID_ReaderContext context;
        PTEID_ReaderSet readerSet;
        readerSet = PTEID_ReaderSet.instance();
        for (int i = 0; i < readerSet.readerCount();
             i++) {
            context = readerSet.getReaderByNum(i);
            if (context.isCardPresent()) {
                card = context.getEIDCard();
                PTEID_EId eid = card.getID();
                String nome = eid.getGivenName();
                String nrCC = eid.getDocumentNumber();
                System.out.println(nome + " " + nrCC);
                ccNumber = nrCC.replaceAll(" ", "").trim();
                System.out.println(ccNumber);
                PTEID_Certificate signature = card.getAuthentication();
                if (signature.isFromPteidValidChain()) {
                } else {

                    System.out.println("Not VALID");
                }

            }
        }


        PTEID_ReaderSet.releaseSDK();

    }


    public static boolean loadPteidLib() throws UnsatisfiedLinkError {
        return ((Boolean) AccessController.doPrivileged(new PrivilegedAction() {
            @Override
            public Boolean run() {
                try {
                    System.loadLibrary("pteidlibj");
                    return true;
                } catch (UnsatisfiedLinkError t) {
                    if (t.getMessage().contains("already loaded")) {
                        JOptionPane.showMessageDialog(null, "Biblioteca do Cartão de Cidadão bloqueada.", "Biblioteca bloqueada", JOptionPane.ERROR_MESSAGE);
                    } else {
                        JOptionPane.showMessageDialog(null, "Middleware do Cartão de Cidadão não está instalado.", "Aplicação não está instalada", JOptionPane.ERROR_MESSAGE);
                    }
                    return false;
                }
            }
        }));
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
                Message message = new Message("Auction", "addAuction", stringAuction);
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

        openUpdateButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

               refreshOpenAuctions();

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
                textFieldUUID.setText(uuid);

            }
        });
        bidButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {

                Gson gson = new Gson();


                Bid bid = new Bid(textFieldUUID.getText(),bidValue.getText(),ccNumber);

                String stringBid = gson.toJson(bid);
                System.out.println(stringBid);
                Message message = new Message("Bid", "addBid", stringBid);
                String stringMessage = gson.toJson(message);
                System.out.println("string message:" + stringMessage);
                try {
                    String result = postMessage(stringMessage, RepositoryPort);
                    System.out.println("result"+result);
                    editorPane3.setText(editorPane3.getText() + "\n\n" + result);
                } catch (Exception en) {
                }

                }

        });

        tabbedPane1.addFocusListener(new FocusAdapter() {
            @Override
            public void focusGained(FocusEvent e) {
                super.focusGained(e);
                refreshOpenAuctions();

            }
        });
        tabbedPane1.addFocusListener(new FocusAdapter() {
            @Override
            public void focusGained(FocusEvent e) {
                super.focusGained(e);
                refreshClosedAuctions();
            }
        });
        closedUpdateButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                refreshClosedAuctions();
            }
        });

        terminateButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                String chosen_auction = comboBoxTerminate.getSelectedItem().toString();
                String uuid = (chosen_auction.substring(chosen_auction.indexOf("<")+1));
                String response="";
                Gson gson = new Gson();
                uuid = uuid.substring(0, uuid.length() - 1);
                Message message = new Message("Auction", "terminateAuction", uuid);
                String stringMessage = gson.toJson(message);
                try {
                    //sends the auction in to the manager to close it
                    response = postMessage(stringMessage, ManagerPort);
                }
                catch (Exception e123){ }

               refreshClosedAuctions();
                refreshOpenAuctions();

            }

        });
        comboBoxTerminate.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {}


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
    private void refreshOpenAuctions()
    {
        String stringMessage = "";


            try {
                stringMessage = (getMessage("open", RepositoryPort));
            } catch (Exception en2) {
            }

        if(stringMessage.equals("empty"))
        {
            editorPaneOpenAuctionsList.setText(" No auctions available...");
        }
        else {
            Gson gson = new Gson();
            Type type = new TypeToken<HashMap<String, ArrayList<Block>>>() {
            }.getType();

            System.out.println("response:" + stringMessage);
            HashMap<String, ArrayList<Block>> active_blockchains = gson.fromJson(stringMessage, type);

            System.out.println(active_blockchains.size());
            editorPaneOpenAuctionsList.setText("");
            for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                String key = entry.getKey();
                ArrayList<Block> value = entry.getValue();
                System.out.println(value.get(0).auction.getProduct());
                String seller = value.get(0).auction.getSeller();
                String product = value.get(0).auction.getProduct();
                String uuid = key;
                String bids = "";
                for (int i = 1; i < value.size(); i++) {
                    bids = " Bid id:" + value.get(i).bid.getBid_id() + "\t(" + value.get(i).bid.getBid_value() + ")€ from: " + value.get(i).bid.getFrom() + " on: " + value.get(i).timestamp + "\n" + bids;
                }
                editorPaneOpenAuctionsList.setText(editorPaneOpenAuctionsList.getText() + "\n\n" + "Seller: " + seller + "  Product: " + product + " UUID:<" + uuid + "> \n" + bids);
                // ...
            }



            comboBoxTerminate.removeAllItems();
            for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                String key = entry.getKey();
                ArrayList<Block> value = entry.getValue();
                String product = (value.get(0).auction.getProduct());
                String uuid = (value.get(0).auction.getId());
                comboBoxTerminate.addItem(product+"<"+uuid+">");
            }
        }

    }
    private void refreshClosedAuctions()
    {
        String stringMessage = "";

        try {
            stringMessage = (getMessage("closed",RepositoryPort));
        } catch (Exception en2) {
        }

            if(stringMessage.equals("empty"))
            {
                editorPaneClosedAuctionsList.setText(" No auctions available...");
            }
            else {
                Gson gson = new Gson();
                Type type = new TypeToken<HashMap<String, ArrayList<Block>>>() {
                }.getType();

                System.out.println("response:" + stringMessage);
                HashMap<String, ArrayList<Block>> active_blockchains = gson.fromJson(stringMessage, type);

                System.out.println(active_blockchains.size());
                editorPaneClosedAuctionsList.setText("");
                for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                    String key = entry.getKey();
                    ArrayList<Block> value = entry.getValue();
                    System.out.println(value.get(0).auction.getProduct());
                    String seller = value.get(0).auction.getSeller();
                    String product = value.get(0).auction.getProduct();
                    String uuid = key;
                    String bids = "";
                    for (int i = 1; i < value.size(); i++) {
                        bids = " Bid id:" + value.get(i).bid.getBid_id() + "\t(" + value.get(i).bid.getBid_value() + ")€ from: " + value.get(i).bid.getFrom() + " on: " + value.get(i).timestamp + "\n" + bids;
                    }
                    editorPaneClosedAuctionsList.setText(editorPaneClosedAuctionsList.getText() + "\n\n" + "Seller: " + seller + "  Product: " + product + " UUID:<" + uuid + "> \n" + bids + "\n Result: TBD");
                    // ...
                }
            }

    }


}



