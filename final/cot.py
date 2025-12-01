import os
import re
import json
import requests
from plantuml import PlantUML

# --------------------------------------------------------------------
# REQUIREMENT DESCRIPTIONS (LOOP OVER THESE)
# --------------------------------------------------------------------

all_descriptions = [
    {
        "title": "Project Management System",
        "desc": "A project manager uses the project management system to manage a project. The project manager leads a team to execute the project within the project's start and end dates. Once a project is created in the project management system, a manager may initiate and later terminate the project due to its completion or for some other reason. As input, a project uses requirements. As output, a project produces a system (or part of a system). The requirements and system are work products: things that are created, used, updated, and elaborated on throughout a project. Every work product has a description, is of some percent complete throughout the effort, and may be validated. However, validation is dependent on the type of work product. For example, the requirements are validated with users in workshops, and the system is validated by being tested against the requirements. Furthermore, requirements may be published using various types of media, including on an intranet or in paper form; and systems may be deployed onto specific platforms."
    },
    {
        "title": "Hollywood Approach",
        "desc": "We are interested in building a software application to manage filmed scenes for realizing a movie, by following the so-called “Hollywood Approach”. Every scene is identified by a code (a string) and it is described by a text in natural language. Every scene is filmed from different positions (at least one), each of this is called a setup. Every setup is characterized by a code (a string) and a text in natural language where the photographic parameters are noted (e.g., aperture, exposure, focal length, filters, etc.). Note that a setup is related to a single scene. For every setup, several takes may be filmed (at least one). Every take is characterized by a (positive) natural number, a real number representing the number of meters of film that have been used for shooting the take, and the code (a string) of the reel where the film is stored. Note that a take is associated to a single setup. Scenes are divided into internals that are filmed in a theater, and externals that are filmed in a location and can either be “day scene” or “night scene”. Locations are characterized by a code (a string) and the address of the location, and a text describing them in natural language."
    },
    {
        "title": "Word Processor",
        "desc": "A user can open a new or existing document. Text is entered through a keyboard. A document is made up of several pages and each page is made up of a header, body and footer. Date, time and page number may be added to header or footer. Document body is made up of sentences, which are themselves made up of words and punctuation characters. Words are made up of letters, digits and/or special characters. Pictures and tables may be inserted into the document body. Tables are made up of rows and columns and every cell in a table can contain both text and pictures. Users can save or print documents."
    },
    {
        "title": "Patient Record and Scheduling System",
        "desc": "A patient record and scheduling system in a doctor’s office is used by the receptionists, nurses, and doctors. The receptionists use the system to enter new patient information when first-time patients visit the doctor. They also schedule all appointments. The nurses use the system to keep track of the results of each visit including diagnosis and medications. For each visit, free form text fields are used captures information on diagnosis and treatment. Multiple medications may be prescribed during each visit. The nurses can also access the information to print out a history of patient visits. The doctors primarily use the system to view patient history. The doctors may enter some patient treatment information and prescriptions occasionally, but most frequently they let the nurses enter this information. -- Each patient is assigned to a family. The head of family is responsible for the person with the primary medical coverage. Information about doctors is maintained since a family has a primary care physician, but different doctors may be the ones seeing the patient during the visit."
    },
    {
        "title": "Movie-Shop",
        "desc": "♣ Design a system for a movie-shop, in order to handle ordering of movies and browsing of the catalogue of the store, and user subscriptions with rechargeable cards. ♣ Only subscribers are allowed hiring movies with their own card. ♣ Credit is updated on the card during rent operations. ♣ Both users and subscribers can buy a movie and their data are saved in the related order. ♣ When a movie is not available it is ordered ."
    },
    {
        "title": "Flights",
        "desc": "We want to model a system for management of flights and pilots. An airline operates flights. Each airline has an ID. Each flight has an ID a departure airport and an arrival airport: an airport as a unique identifier. Each flight has a pilot and a co-pilot, and it uses an aircraft of a certain type; a flight has also a departure time and an arrival time. An airline owns a set of aircrafts of different types. An aircraft can be in a working state or it can be under repair. In a particular moment an aircraft can be landed or airborne. A company has a set of pilots: each pilot has an experience level: 1 is minimum, 3 is maximum. A type of aeroplane may need a particular number of pilots, with a different role (e.g.: captain, co-pilot, navigator): there must be at least one captain and one co-pilot, and a captain must have a level 3."
    },
    {
        "title": "Bank System",
        "desc": "A bank system contains data on customers (identified by name and address) and their accounts. Each account has a balance and there are 2 type of accounts: one for savings which offers an interest rate, the other for investments, used to buy stocks. Stocks are bought at a certain quantity for a certain price (ticker) and the bank applies commission on stock orders."
    },
    {
        "title": "Veterinary Clinic",
        "desc": "The owner of a veterinary clinic wants to create a database to store information about all veterinary services performed. After some research he came up with the following requirements: ● For each admitted animal, its name, breed (if any) and owner must be stored. Each animal should be given an unique numeric identifier. ● For each owner, its name, address and phone number should be stored. An unique numeric identifier should also be generated for each one of them. ● An animal might be owner-less. This happens frequently as the clinic often rescues abandoned dogs from the streets in order to treat them and get them new owners. ● It should be possible to store information about a specific breed even if no animals of that breed have been treated at the clinic. ● Each appointement always has a responsible physician. All appointements start at a certain date and time; and are attended by an animal (and of course its owner). ● For each physician, his name, address and phone number should be stored. An unique numeric identifier should also be generated for each one of them. ● In an appointement, several medical conditions might be detected. Each condition has a common name and a scientific name. No two conditions have the same scientific name. ● It should be possible to store information about the most common conditions for each different breed in the database."
    },
    {
        "title": "Auto Repair",
        "desc": "An auto repair shop, that sells and mounts parts and accessories for all kinds of vehicles, wants a new information system to manage their clients, parts, accessories and assembly services: ● There are several employees. Each one of them has an unique identifying number, a name and an address. ● In this shop, assembly services, where parts and accessories are installed in a vehicle, are executed. For each one these services the following data must be stored: In which car the service was executed, how many kms had the car at the time, who was the responsible employee, which parts and accessories were fitted, how many work hours did it take and the admission and finish dates. ● Parts and accessories are only sold together with an assembly service. ● Each part/accessory only fits in some car models. Therefore, it is important to store that information. ● Each part/accessory has a category (radio, tyre, …), a serial number and a price. ● Each car has a license plate, a make, a model, a color and an owner. Each owner has a name, identifying number, address and a phone. ● One person can own more than one car but one car only has one owner."
    },
    {
        "title": "Restaurant",
        "desc": "The owner of a small restaurant wants a new information system to store data for all meals consumed there and also to keep a record of ingredients kept in stock. After some research he reached the following requirements list: ● Each ingredient has a name, a measuring unit (e.g. olive oil is measured in liters, while eggs are unit based) and a quantity in stock. There are no two ingredients with the same name. ● Each dish is composed of several ingredients in a certain quantity. An ingredient can, of course, be used in different dishes. ● A dish has an unique name and a numeric identifier. ● There are several tables at the restaurant. Each one of them has an unique numeric identifier and a maximum ammount of people that can be seated there. ● In each meal, several dishes are consumed at a certain table. The same dish can be eaten more than once in the same meal. ● A meal takes place in a certain date and has a start and end time. Each meal has a responsible waiter. ● A waiter has an unique numerical identifier, a name, an address and a phone number. ● In some cases it is important to store information about the client that consumed the meal. A client has a tax identification number, a name and an address."
    },
    {
        "title": "Deliveries",
        "desc": "The owner of a small delivery company plans to have an information system that allows him to save data about his customers and deliveries. After some time studying the problem, he reached the following requirements: ● Each customer has a VAT number, a name, a phone number and an address. There are no two clients with the same VAT number. ● When a customer wants to send a package to another customer, he just has to login to the company website, select the customer he wants to send the package to, enter the package’s weight and if the delivery is normal or urgent. He then receives an unique identifier code that he writes on the package. ● The package is then delivered by the customer at the delivery center of his choosing. A delivery center has a unique name and an address. ● Each client has an associated delivery center. This delivery center is chosen by the company and it is normally the one closest to the customer’s house. ● The package is them routed through an internal system until it reaches the delivery center of the recipient. ● The package is then delivered by hand from that delivery center to the recipient by a courier. ● Couriers have a single VAT number, a name and a phone number. Each courier works in a single delivery center. ● A courier is assigned to a packet as soon as the packet is introduced in the system."
    },
    {
        "title": "Furniture",
        "desc": "The known furniture factory Hi-Key-Ah, intends to implement an information system to store all data on the different types of furniture and components it produces: ● The factory produces several lines of furniture, each with a different name and consisting of several pieces of furniture of different types (beds, tables, chairs, …). ● All furniture pieces have a type, a single reference (eg CC6578) and a selling price. ● The major competitive advantage of this innovative plant is the fact that each component produced can be used in more than one piece of furniture. ● Each piece of furniture is thus composed of several components. The same component can be used more than once in the same piece. ● Every type of component produced is assigned a unique numerical code, a manufacturing price and a type (screw, hinge, shelf …). ● The furniture is then sold in various stores throughout the world. Each store has a different address and a fax number. ● To make the manufacturing process more efficient, stores have to place orders everytime they need to replenish their stock. These orders must also be stored in the database. ● Each order has a order number, a date, the store that placed the order as well as a list of all the ordered furniture and their quantities."
    },
    {
        "title": "Factory",
        "desc": "Create a database for a factory with the following requirements. Don’t forget to add unique identifiers for each one of the entities if needed. ● A factory has several machines. Each one of them is operated by several workers. ● A worker might work in more than one machine. ● In this factory, several products of different types, are produced. Each different type of product is produced in a single machine. But, the same machine can produce more than one type of product. ● Products from the same type are all produced from the same single material and have the same weigth. ● Clients can issue purchase orders. Each order has a list of the desired products and their quantity. ● For each worker, the following data should be stored in the database: name (first and last), birth date, address and a list of his skills. ● For each machine, the following data should be stored: serial number, make, model and purchase date. ● For each client, the followig data should be stored: name, address, phone number and name of the contact person (if any). ● For each purchase order, the following date should be stored: order number, date it has been made, expected and actual delivery date."
    },
    {
        "title": "Bycicle Rental",
        "desc": "A bicycle renting company wants to create an information system that allows it to store the data regarding all their reservations and rentals. The system should follow these requirements: ● It should be possible to store the national id number (NIN), tax identification number (TIN), name and address for every client. The NIN and TIN must be different for every client and all clients should have at least a TIN and a name. ● The database should also contain information about the bicycle models that can be rented- Each model has an unique name, a type (that can only be road, mountain, bmx or hybrid) and the number of gears. ● Each bicycle has a unique identifying number and a model. ● The company has several different stores where bicycles can be picked up and returned. Each one of these stores is identified by an unique name and has an address (both mandatory). ● When a reservation is made, the following data must be known: which client did the reservation, when will he pick up the bike (day), which bike model he wants and where will he pick up the bike (store). ● When a bike is picked up, the actual bike that was picked up must be stored in the database. ● When a bike is returned, the return date should be stored in the database."
    },
    {
        "title": "Saturn Int. Management",
        "desc": "Saturn Int. management wants to improve their security measures, both for their building and on site. They would like to prevent people who are not part of the company to use their car park. Saturn Int. has decided to issue identity cards to all employees. Each card records the name, department and number of a company staff, and give them access to the company car park. Employees are asked to wear the cards while on the site. There is a barrier and a card reader placed at the entrance to the car park. When a driver drives his car into the car park, he/she inserts his or her identity card into the card reader. The card reader then verify the card number to see if it is known to the system. If the number is recognized, the reader sends a signal to trigger the barrier to rise. The driver can then drive his/her car into the car park. There is another barrier at the exit of the car park, which is automatically raised when a car wishes to leave the car park. A sign at the entrance display “Full” when there are no spaces in the car park. It is only switched off when a car leaves. There is another type of card for guests, which also permits access to the car park. The card records a number and the current date. Such cards may be sent out in advance, or collected from reception. All guest cards must be returned to reception when the visitor leaves Saturn Int."
    },
    {
        "title": "OOBank",
        "desc": "This system provides the basic services to manage bank accounts at a bank called OOBank. OOBank has many branches, each of which has an address and branch number. A client opens accounts at a branch. Each account is uniquely identified by an account number; it has a balance and a credit or overdraft limit. There are many types of accounts, including: a mortgage account (which has a property as collateral), a checking account, and a credit card account (which has an expiry date and can have secondary cards attached to it). It is possible to have a joint account (e.g. for a husband and wife). Each type of account has a particular interest rate, a monthly fee and a specific set of privileges (e.g. ability to write checks, insurance for purchases etc.). OOBank is divided into divisions and subdivisions (such as Planning, Investments and Consumer); the branches are considered subdivisions of the Consumer Division. Each division has a manager and a set of other employees. Each customer is assigned a particular employee as his or her ‘personal banker’."
    },
    {
        "title": "Prepaid Cell Phone",
        "desc": "The contract of a prepaid cell phone should be modelled and implemented. A basic contract has a contract number (of type int) and a balance (of type double), but no monthly charges. The contract number is not automatically generated, but is to be set as a parameter by the constructor as well as the initial balance. The balance has a getter and a setter. The following options can be added to a contract (if needed also several times): ● 100 MB of data (monthly charge 1.00€) ● 50 SMS (monthly charge 0.50€) ● 50 minutes (monthly charge 1.50€) ● Double Transfer Rate (monthly charge 2.00€) implement this requirement with the help of the decorator pattern. All contract elements should be able to understand the methods getCharges():double, getBalance():double and setBalance(double). The method getCharges() should provide the monthly charge of a contract with all its options selected. The methods getBalance() and setBalance() should be passed through and access the basic contract."
    },
    {
        "title": "Library System",
        "desc": "The exercise id to design a class structure for a library system. It should fulfil those requirements: ● There are two type of users – under-aged and adults. ● Under-aged users are identified with usage of their full name and student card. ● Adult users are identified with usage of their full name and ID card. ● The library contains books. ● There is basic information about every book (title, author, etc). ● The user can borrow at most 4 books at the same time. ● There is a history of previously borrowed books for every user (along with all the dates)."
    },
    {
        "title": "MyDoctor",
        "desc": "The MyDoctor application aims to be a management tool for the appointments of a doctor. A hospital has multiple offices. The users of the application can be doctors and patients. The doctors can apply to practice in offices and create a schedule for an office. The schedules in different offices can’t overlay. 📝 Example: Doctor Ana is available in Office 4 on the 4th of September during 1 PM - 5PM. Doctor Ana can’t practice in Office 5 on the 4th of September during 3PM - 8 PM, but she can practice in Office 5 on the 4th of September during 5:30PM - 8 PM. The patients can see the existing doctors in the system, the schedule of the offices and can book appointments for specific doctors and for specific schedules. The appointments can be of 3 types: ● Blood Test - 15 mins ● Consultation - 30 mins ● Surgery - 60 mins The booking of an appointment will not be possible if another appointment is already booked at the same time frame. An email is sent to the patient with the confirmation of the appointment. 📝 Example: Action 1: User Mike will create a blood test booking for Doctor Ana for the 4th of September starting with 15:30 PM → Possible Action 2: User Mike will create an intervention booking for Doctor Ana for the 4th of September starting with 15:00 PM → Not Possible Action 3: User Mike will create an intervention booking for Doctor Ana for the 4th of September starting with 16:00 PM → Possible"
    },
    {
        "title": "Online Shopping",
        "desc": "Each customer has unique id and is linked to exactly one account. Account owns shopping cart and orders. Customer could register as a web user to be able to buy items online. Customer is not required to be a web user because purchases could also be made by phone or by ordering from catalogues. Web user has login name which also serves as unique id. Web user could be in several states - new, active, temporary blocked, or banned, and be linked to a shopping cart. Shopping cart belongs to account. Account owns customer orders. Customer may have no orders. Customer orders are sorted and unique. Each order could refer to several payments, possibly none. Every payment has unique id and is related to exactly one account. Each order has current order status. Both order and shopping cart have line items linked to a specific product. Each line item is related to exactly one product. A product could be associated to many line items or no item at all."
    }
]

# --------------------------------------------------------------------
# CONFIGURATION
# --------------------------------------------------------------------

# Get key from env var (recommended)
OPENAI_API_KEY = "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"
OPENAI_API_URL = "https://api.openai.com/v1/chat/completions"
PLANTUML_SERVER_URL = "http://www.plantuml.com/plantuml/png/"

# Root output dir; each requirement gets its own subdir
OUTPUT_DIR = "cot"
os.makedirs(OUTPUT_DIR, exist_ok=True)

COT_RESULTS_JSON_FILE = os.path.join(OUTPUT_DIR, "cot_results.json")


# --------------------------------------------------------------------
# 1. LLM CALLER (OpenAI Version)
# --------------------------------------------------------------------

def _openai_chat(
    prompt: str,
    *,
    system: str | None = None,
    model: str = "gpt-4o-mini",
    temperature: float = 0.2,
    max_tokens: int = 4000,
) -> str | None:
    """Low-level wrapper for OpenAI chat completions."""
    if not OPENAI_API_KEY:
        print("[ERROR] OPENAI_API_KEY not set.")
        return None

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {OPENAI_API_KEY}",
    }

    messages = []
    if system:
        messages.append({"role": "system", "content": system})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": model,
        "messages": messages,
        "max_tokens": max_tokens,
        "temperature": temperature,
    }

    try:
        resp = requests.post(
            OPENAI_API_URL, headers=headers, data=json.dumps(payload), timeout=60
        )
        resp.raise_for_status()
        j = resp.json()
        if "choices" in j and j["choices"]:
            return j["choices"][0].get("message", {}).get("content")
    except Exception as e:
        print("OpenAI request failed:", e)
        return None

def call_llm(prompt: str, system: str = None) -> str:
    """High-level LLM call used in the pipeline."""
    text = _openai_chat(prompt, system=system)
    return text or ""

# --------------------------------------------------------------------
# 2. HELPER FUNCTIONS
# --------------------------------------------------------------------

def log_step(step_name, content, file_handle):
    """Prints to console and writes to log file."""
    header = f"\n{'='*20} {step_name} {'='*20}\n"
    # print(header)
    # print(content[:300] + "..." if len(content) > 300 else content)
    file_handle.write(header)
    file_handle.write(content + "\n\n")
    file_handle.flush()

def clean_plantuml_code(text):
    """Extracts code between @startuml and @enduml."""
    match = re.search(r'@startuml.*?@enduml', text, re.DOTALL)
    if match:
        return match.group(0).strip()
    return text.strip()

def slugify(title: str) -> str:
    s = title.strip().lower()
    s = re.sub(r'[^a-z0-9]+', '_', s)
    return s.strip('_') or "requirement"

# --------------------------------------------------------------------
# 3. CHAIN OF THOUGHT STEPS
# --------------------------------------------------------------------

def step1_identify_classes(desc):
    prompt = f"""
    You are a UML modeling expert. Identify the main classes/entities from this description.
    Focus on NOUNS that represent real-world entities. Ignore implementation details.
    
    Description:
    {desc}
    
    Output a list of classes with brief justification for each.
    """
    return call_llm(prompt)

def step2_attributes_methods(desc, classes_info):
    prompt = f"""
    Based on the identified classes, identify attributes (properties) and methods (behaviors).
    
    Classes Identified:
    {classes_info}
    
    Description:
    {desc}
    
    For each class, list:
    - Attributes (Name: Type)
    - Methods (Name(): ReturnType)
    """
    return call_llm(prompt)

def step3_relationships(desc, classes_info, attr_info):
    prompt = f"""
    Identify relationships between the classes (Association, Aggregation, Composition, Inheritance).
    Determine cardinalities (1, 0..1, *, etc.).
    
    Context:
    {classes_info}
    {attr_info}
    
    Description:
    {desc}
    
    List relationships in format: ClassA -- RelationshipType -- ClassB : Cardinality
    """
    return call_llm(prompt)

def step4_generate_textual_uml(desc, s1, s2, s3):
    prompt = f"""
    Synthesize the previous steps into a structured Textual UML description.
    
    Inputs:
    1. Classes: {s1}
    2. Attrs/Methods: {s2}
    3. Relationships: {s3}
    
    Description: {desc}
    
    Format output exactly like this:
    CLASS: [Name]
    Description: [Text]
    ATTRIBUTES:
    - [name]: [type]
    METHODS:
    - [name](): [type]
    
    RELATIONSHIPS:
    - [ClassA] [rel_type] [ClassB] [cardinality]
    """
    return call_llm(prompt)

def step5_generate_plantuml(textual_uml):
    prompt = f"""
    Convert this Textual UML into valid PlantUML syntax.
    
    Rules:
    - Start with @startuml, end with @enduml
    - Use class definitions
    - Use correct relationship arrows (<|--, *--, o--, -->)
    - No markdown ticks.
    
    Input:
    {textual_uml}
    """
    return call_llm(prompt)

# --------------------------------------------------------------------
# 4. MAIN EXECUTION (LOOP OVER ALL REQUIREMENTS)
# --------------------------------------------------------------------

def run_cot_generation_for_all2():
    if not OPENAI_API_KEY:
        print("Please set your OPENAI_API_KEY environment variable.")
        return

    print(f"Root Output Directory: {OUTPUT_DIR}")

    for item in all_descriptions[:2]:
        title = item["title"]
        desc = item["desc"]
        slug = slugify(title)
        req_dir = os.path.join(OUTPUT_DIR, slug)
        os.makedirs(req_dir, exist_ok=True)

        process_log_path = os.path.join(req_dir, "cot_process_log.txt")
        textual_uml_path = os.path.join(req_dir, "textual_uml.txt")
        plantuml_code_path = os.path.join(req_dir, "class_diagram.puml")
        diagram_image_path = os.path.join(req_dir, "class_diagram.png")

        print("\n" + "#" * 70)
        print(f"Starting CoT Process for: {title}")
        print(f"Requirement folder: {req_dir}")
        print("#" * 70 + "\n")

        with open(process_log_path, "w", encoding="utf-8") as log:
            log.write(f"PROJECT: {title}\n")
            log.write(f"REQ:\n{desc}\n\n")

            # --- Step 1: Classes ---
            s1 = step1_identify_classes(desc)
            log_step("Step 1: Identify Classes", s1, log)

            # --- Step 2: Attrs/Methods ---
            s2 = step2_attributes_methods(desc, s1)
            log_step("Step 2: Attributes & Methods", s2, log)

            # --- Step 3: Relationships ---
            s3 = step3_relationships(desc, s1, s2)
            log_step("Step 3: Relationships", s3, log)

            # --- Step 4: Textual UML ---
            textual_uml = step4_generate_textual_uml(desc, s1, s2, s3)
            log_step("Step 4: Textual UML", textual_uml, log)

        # Save Textual UML separately
        with open(textual_uml_path, "w", encoding="utf-8") as f:
            f.write(textual_uml)

        # --- Step 5: PlantUML Code ---
        raw_puml = step5_generate_plantuml(textual_uml)
        clean_puml = clean_plantuml_code(raw_puml)

        with open(process_log_path, "a", encoding="utf-8") as log:
            log_step("Step 5: PlantUML Code", clean_puml, log)

        # Save PlantUML Code
        with open(plantuml_code_path, "w", encoding="utf-8") as f:
            f.write(clean_puml)

        # --- Step 6: Rendering Diagram ---
        print(f"\n...Rendering Diagram via PlantUML Server for {title}...")
        try:
            server = PlantUML(url=PLANTUML_SERVER_URL)
            png_data = server.processes(clean_puml)
            with open(diagram_image_path, "wb") as f:
                f.write(png_data)
            print(f"[SUCCESS] Diagram saved to: {diagram_image_path}")
        except Exception as e:
            print(f"[ERROR] Failed to render image for {title}: {e}")


def run_cot_generation_for_all():
    if not OPENAI_API_KEY:
        print("Please set your OPENAI_API_KEY environment variable.")
        return

    print(f"Root Output Directory: {OUTPUT_DIR}")

    # Collect all textual UMLs here in the same order as all_descriptions  ### NEW
    cot_results = []                                                     ### NEW

    for item in all_descriptions:
        title = item["title"]
        desc = item["desc"]
        slug = slugify(title)
        req_dir = os.path.join(OUTPUT_DIR, slug)
        os.makedirs(req_dir, exist_ok=True)

        process_log_path = os.path.join(req_dir, "cot_process_log.txt")
        textual_uml_path = os.path.join(req_dir, "textual_uml.txt")
        plantuml_code_path = os.path.join(req_dir, "class_diagram.puml")
        diagram_image_path = os.path.join(req_dir, "class_diagram.png")

        print("\n" + "#" * 70)
        print(f"Starting CoT Process for: {title}")
        # print(f"Requirement folder: {req_dir}")
        print("#" * 70 + "\n")

        with open(process_log_path, "w", encoding="utf-8") as log:
            log.write(f"PROJECT: {title}\n")
            log.write(f"REQ:\n{desc}\n\n")

            # --- Step 1: Classes ---
            s1 = step1_identify_classes(desc)
            log_step("Step 1: Identify Classes", s1, log)

            # --- Step 2: Attrs/Methods ---
            s2 = step2_attributes_methods(desc, s1)
            log_step("Step 2: Attributes & Methods", s2, log)

            # --- Step 3: Relationships ---
            s3 = step3_relationships(desc, s1, s2)
            log_step("Step 3: Relationships", s3, log)

            # --- Step 4: Textual UML ---
            textual_uml = step4_generate_textual_uml(desc, s1, s2, s3)
            log_step("Step 4: Textual UML", textual_uml, log)

        # Save Textual UML separately (per requirement)
        with open(textual_uml_path, "w", encoding="utf-8") as f:
            f.write(textual_uml)

        # ---- ADD THIS BLOCK TO COLLECT RESULTS IN ARRAY -----------------  ### NEW
        cot_results.append({
            "title": title,
            "description": desc,
            "textual_uml": textual_uml
        })
        # ------------------------------------------------------------------  ### NEW

        # --- Step 5: PlantUML Code ---
        raw_puml = step5_generate_plantuml(textual_uml)
        clean_puml = clean_plantuml_code(raw_puml)

        with open(process_log_path, "a", encoding="utf-8") as log:
            log_step("Step 5: PlantUML Code", clean_puml, log)

        # Save PlantUML Code
        with open(plantuml_code_path, "w", encoding="utf-8") as f:
            f.write(clean_puml)

        # --- Step 6: Rendering Diagram ---
        print(f"\n...Rendering Diagram via PlantUML Server for {title}...")
        try:
            server = PlantUML(url=PLANTUML_SERVER_URL)
            png_data = server.processes(clean_puml)
            with open(diagram_image_path, "wb") as f:
                f.write(png_data)
            print(f"[SUCCESS] Diagram saved to: {diagram_image_path}")
        except Exception as e:
            print(f"[ERROR] Failed to render image for {title}: {e}")

    # --- AFTER THE LOOP: SAVE THE JSON ARRAY -----------------------------  ### NEW
    with open(COT_RESULTS_JSON_FILE, "w", encoding="utf-8") as jf:
        json.dump(cot_results, jf, indent=2, ensure_ascii=False)
    print(f"\n[INFO] All textual UMLs saved to JSON: {COT_RESULTS_JSON_FILE}")  ### NEW
    # ----------------------------------------------------------------------  ### NEW

if __name__ == "__main__":
    run_cot_generation_for_all()
