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

    String ccNumber = "12345678";
    Integer RepositoryPort = 8501;
    Integer ManagerPort = 8500;
    public static void main(String[] args) throws Exception {

    //Start the GUI
        JFrame frame = new JFrame("Auction Client");
        frame.setContentPane(new AuctionClient().panel1);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);



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
                    String result = send(stringMessage,ManagerPort);
                    System.out.println(result);
                    editorPane1.setText( editorPane1.getText() + "\n\n" +  result);
                }catch (Exception en) {
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
                try {
                    ask(stringMessage,RepositoryPort);
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

    private String send(String message2send,Integer port) throws Exception{


        // HTTP code
        StringEntity entity = new StringEntity(message2send,
                ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build(); //todo; change ports send(message, port)
        HttpPost request = new HttpPost("http://localhost:"+port+"/security"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE
        request.setEntity(entity);


        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());
        return responseBody;
    }
    private void ask(String what,Integer port) throws Exception{
        // HTTP code
        StringEntity entity = new StringEntity(what, ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpGet request = new HttpGet("http://localhost:"+port+"/security"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE

        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());

        GETtextplane.setText(responseBody + "\n\n" + GETtextplane.getText());
    }
}



