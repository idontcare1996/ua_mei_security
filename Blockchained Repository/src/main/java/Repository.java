import com.google.gson.Gson;
import com.sun.net.httpserver.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URI;
import java.nio.charset.Charset;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Scanner;



public class Repository {

    //Create ArrayList, named blockchain with objects of type Block
    public  static ArrayList<Block> blockchain = new ArrayList<Block>();

    //Ammount of blocks to add to the blockchain, for testing
    private static final int BLOCKSTOADD = 4;

    public static void main(String[] args) throws Exception {

        // BLOCKCHAIN STUFF
        /*
        //Adding the genesis block to the blockchain
        blockchain.add(new Block("Data1","0")); // First block's previous hash is 0 because it's the genesis block.

        //Adding remaining blocks to the blockchain, and setting the "previous_hash" to the previous item in the blockchain's hash.
        for(int i=1; i < BLOCKSTOADD; i++) {
            blockchain.add(new Block("Data"+i, blockchain.get(blockchain.size() - 1).hash));
        }

        //Adds an element in the blockchain to invalidade it. It gets between position 1 and 2.
        //blockchain.add(2, new Block("Data0",blockchain.get(1).hash));


        //Stores the blockchain in JSON format, with PrettyPrinting on(facilitating human reading).
        String blockchainJSON = new GsonBuilder().setPrettyPrinting().create().toJson(blockchain);

        //Prints the JSON
        System.out.println(blockchainJSON);

        System.out.println("Is the blockchain valid: "+isBlockChainValid());
        */

        // HTTP SERVER STUFF
        HttpServer server = HttpServer.create(new InetSocketAddress(8500), 0);
        HttpContext context = server.createContext("/example");
        context.setHandler(Repository::handleRequest);
        server.start();






    }
    private static void handleRequest(HttpExchange exchange) throws IOException {
        Timestamp timestampReception = new Timestamp(System.currentTimeMillis());
        URI requestURI = exchange.getRequestURI();
        //printRequestInfo(exchange);
        //String response = "This is the response at " + requestURI;
        if(exchange.getRequestMethod().equalsIgnoreCase("POST"))
        {
            System.out.println( " \n          POST ");
            String received = new String();

            received = convert(exchange.getRequestBody(),Charset.forName("UTF-8"));

            Gson gsonreceived = new Gson();

            Auction receivedAuction = gsonreceived.fromJson(received,Auction.class);
            String response = ("Auction id: " + receivedAuction.getId() + "\n" + "Auction creation timestamp: " + receivedAuction.getTimestamp() + "\n" + "Timestamp reception: " + timestampReception + "\n");
            System.out.println(response);
            //USAR O receivedAuction.get(...) para usar os dados no repositório e no manager

            //envio a resposta ao pedido de Post, neste caso, metendo esta string construída com o que se obtem do novo objecto Auction
            exchange.sendResponseHeaders(200, response.length());
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();


        }

        if(exchange.getRequestMethod().equalsIgnoreCase("GET"))
        {
            System.out.println( " GET ");

            String received = new String();
            String response = new String();

            //Figure out what they want with this GET method
            //TO DO: DO SOMETHING WITH THE GET REQUEST

            response = "We received your GET request. Unfortunately, Madaíl hasn't done this part yet and I'm tired \n " + "Time: " + timestampReception;
            System.out.println(response);
            exchange.sendResponseHeaders(200, response.getBytes().length);
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }



    }

    private static void printRequestInfo(HttpExchange exchange) {
        System.out.println("-- headers --");
        Headers requestHeaders = exchange.getRequestHeaders();
        requestHeaders.entrySet().forEach(System.out::println);

        System.out.println("-- principle --");
        HttpPrincipal principal = exchange.getPrincipal();
        System.out.println(principal);

        System.out.println("-- HTTP method --");
        String requestMethod = exchange.getRequestMethod();
        System.out.println(requestMethod);

        System.out.println("-- query --");
        URI requestURI = exchange.getRequestURI();
        String query = requestURI.getQuery();
        System.out.println(query);


    }

    //Method to test blockchain's validity
    public static Boolean isBlockChainValid()
    {
        //Create the Blocks objects to use next
        Block previous_block,current_block;

        //Check each element of the blockchain for it's hash (starts at 1, because the first element has "0" as previous hash - the genesis block
        for(int i=1; i < blockchain.size(); i++){

            current_block = blockchain.get(i);
            previous_block = blockchain.get(i-1);

            //compare the hash on the block to the calculated hash:
            if(!current_block.hash.equals(current_block.calculateHash())){
                System.out.println("Current hash is different for block in position number "+i+" in the blockchain.");
                return false;
            }
            if(!previous_block.hash.equals(current_block.previous_hash)){
                System.out.println("Previous hash is different for block in position number "+i+" in the blockchain.");
                return false;
            }

        }
        return true;
    }
    public static String convert(InputStream inputStream, Charset charset) throws IOException {

        try (Scanner scanner = new Scanner(inputStream, charset.name())) {
            return scanner.useDelimiter("\\A").next();
        }
    }

}
